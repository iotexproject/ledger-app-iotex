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
/* protocol buffer constants */
#define PB_WIRETYPE_MASK    7       /* AND mask to get wire type bits */
#define PB_WT_VI            0       /* wire type: varint */
#define PB_WT_64            1       /* wire type: 64-bit */
#define PB_WT_LD            2       /* wire type: length delimited */
#define PB_WT_32            5       /* wire type: 32-bit */


#define PB_GET_WTYPE(x) ((x) & PB_WIRETYPE_MASK)
#define PB_GET_FIELD(x) (((x) >> 3) & 0x1fffffff)

/* IoTeX protobuf fields for message action core */
#define ACT_VERSION         1       /* version */
#define ACT_NONCE           2       /* nonce */
#define ACT_GASLIMIT        3       /* gasLimit */
#define ACT_GASPRICE        4       /* gasprice */
#define ACT_TRANSFER        10      /* transfer */
#define ACT_EXECUTION       12      /* execution */
#define ACT_STAKE_CREATE	40
#define ACT_STAKE_UNSTAKE	41
#define ACT_STAKE_WITHDRAW	42
#define ACT_STAKE_ADD_DEPOSIT	43
#define ACT_STAKE_RESTAKE	44
#define ACT_STAKE_CHANGE_CDD	45
#define ACT_STAKE_TX_OWNERSHIP	46
#define ACT_STAKE_CDD_REGISTER	47
#define ACT_STAKE_CDD_UPDATE	48

/* Max payload bytes to display */
#define MAX_PAYLOAD_DISPLAY 50

typedef struct {
    uint16_t max_chars_per_key_line;
    uint16_t max_chars_per_value_line;
    const char *tx;
    uint8_t cache_valid;
} parsing_context_t;

uint64_t decode_varint(const uint8_t *buf, uint8_t *skip_bytes, uint8_t max_len);
int decode_pb(const uint8_t *pb_data, uint32_t len, uint32_t *totalfields, int queryid);
