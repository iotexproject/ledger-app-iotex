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

#pragma once

#include "pb_parser.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TX_TOKEN_NOT_FOUND (-1)

#define MAX_RECURSION_DEPTH  6

typedef struct {
    int16_t item_index;             // ??
    int16_t chunk_index;            // ??

    char *out_key;                  // ??
    int16_t out_key_len;            // ??
    char *out_val;                  // ??
    int16_t out_val_len;            // ??
} tx_query_t;

// used to reduce stack size usage in recursive calls
typedef struct {
    tx_query_t query;                       // ??
    int8_t item_index_current;                     // ??
    uint8_t max_level;
    uint8_t max_depth;
    char actiontype;
} tx_context_t;

enum tx_actiontype {
	ACTION_TX = 1,
	ACTION_EXE,
	ACTION_SKT_CREATE,
	ACTION_SKT_UNSTAKE,
	ACTION_SKT_WITHDRAW,
	ACTION_SKT_ADD_DEPOSIT,
	ACTION_SKT_RESTAKE,
	ACTION_SKT_CHANGE_CDD,
	ACTION_SKT_TX_OWNERSHIP,
	ACTION_SKT_CDD_REGISTER,
	ACTION_SKT_CDD_UPDATE,
	ACTION_MAX_INVALID
};

extern parsing_context_t parsing_context;
extern tx_context_t tx_ctx;

#define INIT_QUERY_CONTEXT(_KEY, _KEY_LEN, _VAL, _VAL_LEN, _CHUNK_IDX, _MAX_LEVEL) \
    INIT_QUERY(_KEY, _KEY_LEN, _VAL, _VAL_LEN, _CHUNK_IDX) \
    tx_ctx.item_index_current = 0; \
    tx_ctx.max_depth = MAX_RECURSION_DEPTH; \
    tx_ctx.max_level = _MAX_LEVEL;

#define INIT_QUERY(_KEY, _KEY_LEN, _VAL, _VAL_LEN, _CHUNK_IDX)  \
        _KEY[0] = 0; \
        _VAL[0] = 0; \
        tx_ctx.query.out_key=_KEY; \
        tx_ctx.query.out_val=_VAL; \
        tx_ctx.query.out_key_len = _KEY_LEN; \
        tx_ctx.query.out_val_len = _VAL_LEN; \
        tx_ctx.query.item_index= 0; \
        tx_ctx.query.chunk_index = _CHUNK_IDX;


// Traverses transaction data and fills tx_context
// \return -1 if the item was not found or the number of available chunks for this item
int16_t tx_traverse(int16_t root_token_index);

//---------------------------------------------

#ifdef __cplusplus
}
#endif
