/*******************************************************************************
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

#include "stdint.h"
#include "stdbool.h"

/* Max payload bytes to display */
#define MAX_PAYLOAD_DISPLAY 50

typedef struct {
    uint16_t max_chars_per_key_line;
    uint16_t max_chars_per_value_line;
    const char *tx;
    uint8_t cache_valid;
} parsing_context_t;

typedef enum {
    String = 0x1200,
    Bytes = 0x1300,
    Iotx = 0x1400,
    XRC20Token = 0x1500,
    Bech32Addr = 0x1600,
    Varint = 0x2000,
    Bool = 0x2100,
} field_type_t;

typedef struct {
    const char *key;
    field_type_t type;
    union {
        uint64_t varint;
        struct {
            const char *buf;
            size_t len;
        } ld;
    } data;
} field_display_t;

#define IS_FILED_TYPE_LD(x) ((x) & 0x1000)
#define IS_FIELD_TYPE_VI(x) ((x) & 0x2000)

int decode_pb(const uint8_t *pb_data, uint32_t len, uint32_t *totalfields, int queryid);
