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

#include "transaction.h"
#include "apdu_codes.h"
#include "buffering.h"
#include "tx_parser.h"
#include "pb_parser.h"


#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    #define RAM_BUFFER_SIZE 8192
    #define FLASH_BUFFER_SIZE 16384
#elif defined(TARGET_NANOS)
    #define RAM_BUFFER_SIZE 256
    #define FLASH_BUFFER_SIZE 8192
#endif

// Ram
uint8_t ram_buffer[RAM_BUFFER_SIZE];

// Flash
typedef struct {
    uint8_t buffer[FLASH_BUFFER_SIZE];
} storage_t;

#if defined(TARGET_NANOS)
storage_t N_appdata_impl __attribute__ ((aligned(64)));
#define N_appdata (*(storage_t *)PIC(&N_appdata_impl))

#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
storage_t const N_appdata_impl __attribute__ ((aligned(64)));
#define N_appdata (*(volatile storage_t *)PIC(&N_appdata_impl))
#endif

void transaction_initialize() {
    buffering_init(
        ram_buffer,
        sizeof(ram_buffer),
        N_appdata.buffer,
        sizeof(N_appdata.buffer)
    );
}

void transaction_reset() {
    buffering_reset();
}

uint32_t transaction_append(unsigned char *buffer, uint32_t length) {
    return buffering_append(buffer, length);
}

uint32_t transaction_get_buffer_length() {
    return buffering_get_buffer()->pos;
}

uint8_t *transaction_get_buffer() {
    return buffering_get_buffer()->data;
}

const char* transaction_parse(int *error_code) {
    const uint8_t *transaction_buffer = transaction_get_buffer();

    *error_code = decode_pb(transaction_buffer,transaction_get_buffer_length(), NULL, -1);
    if (*error_code != 0) {
        return "Invalid transaction format";
    }

    parsing_context.cache_valid = 0;
    return NULL;
}
