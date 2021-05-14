/*******************************************************************************
*   (c) ZondaX GmbH
*   (c) 2019 IoTeX Foundation
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include <stdio.h>
#include <string.h>
#include "tx_parser.h"
#include "tx_display.h"
#include "transaction.h"

#if defined(TARGET_NANOS) || defined(TARGET_NANOX)
#include "os.h"
#else
#define PIC(X) X
#endif

// Required pages
// FIXME: the required root items have been moved to a function due to PIC issues. Refactor and fix
const char *get_required_root_item(uint8_t i) {
    switch (i) {
        case 0:
            return "chain_id";
        case 1:
            return "account_number";
        case 2:
            return "sequence";
        case 3:
            return "fee";
        case 4:
            return "memo";
        case 5:
            return "msgs";
        default:
            return "?";
    }
}


static const uint16_t root_max_level[NUM_REQUIRED_ROOT_PAGES] = {
        2, // "chain_id",
        2, // "account_number",
        2, // "sequence",
        1, // "fee",
        2, // "memo"
        2, // "msgs"
};

static const key_subst_t key_substitutions[NUM_KEY_SUBSTITUTIONS] = {
        {"Example Key", "Substituted Key"},
};

static const key_subst_t value_substitutions[NUM_VALUE_SUBSTITUTIONS] = {
        {"Example Value", "Substituted Value"},
};

#define STRNCPY_S(DST, SRC, DST_SIZE) \
    strncpy(DST, SRC, DST_SIZE - 1); \
    DST[DST_SIZE - 1] = 0;

display_cache_t display_cache;

display_cache_t *tx_display_cache() {
    return &display_cache;
}
 void tx_display_index_root() {
    uint32_t totalpages;

    if (parsing_context.cache_valid) {
        return;
    }

    // Clear values
    display_cache.num_pages = 0;
    memset(display_cache.num_subpages, 0, NUM_REQUIRED_ROOT_PAGES);
    memset(display_cache.subroot_start_token, TX_TOKEN_NOT_FOUND, NUM_REQUIRED_ROOT_PAGES);

    decode_pb(transaction_get_buffer(),transaction_get_buffer_length(), &totalpages, -1);

    display_cache.num_pages = totalpages;
    parsing_context.cache_valid = 1;
}

int16_t tx_display_num_pages() {
    tx_display_index_root();
    return display_cache.num_pages;
}

// This function assumes that the tx_ctx has been set properly
int16_t tx_display_get_item(uint16_t page_index) {
    if (!parsing_context.cache_valid) {
        return ERR_MUST_INDEX_FIRST;
    }

    // TODO: Verify it has been properly set?
    tx_ctx.query.out_key[0] = 0;
    tx_ctx.query.out_val[0] = 0;
    if (page_index < 0 || page_index >= display_cache.num_pages) {
        return -1;
    }

    tx_ctx.query.item_index = 0;
    uint16_t root_index = 0;
    for (uint16_t i = 0; i < page_index; i++) {
        tx_ctx.query.item_index++;
        if (tx_ctx.query.item_index >= display_cache.num_subpages[root_index]) {
            tx_ctx.query.item_index = 0;
            root_index++;
        }
    }

    tx_ctx.item_index_current = 0;
    tx_ctx.max_level = root_max_level[root_index];
    tx_ctx.max_depth = MAX_RECURSION_DEPTH;

    STRNCPY_S(tx_ctx.query.out_key, get_required_root_item(root_index), tx_ctx.query.out_key_len);

    int16_t ret = tx_traverse(display_cache.subroot_start_token[root_index]);

    return ret;
}

void tx_display_make_friendly() {
    // post process keys
    for (int8_t i = 0; i < NUM_KEY_SUBSTITUTIONS; i++) {
        if (!strcmp(tx_ctx.query.out_key, key_substitutions[i].str1)) {
            STRNCPY_S(tx_ctx.query.out_key,
                      key_substitutions[i].str2,
                      tx_ctx.query.out_key_len);
            break;
        }
    }

    for (int8_t i = 0; i < NUM_VALUE_SUBSTITUTIONS; i++) {
        if (!strcmp(tx_ctx.query.out_val, value_substitutions[i].str1)) {
            STRNCPY_S(tx_ctx.query.out_val,
                      value_substitutions[i].str2,
                      tx_ctx.query.out_val_len);
            break;
        }
    }

}

