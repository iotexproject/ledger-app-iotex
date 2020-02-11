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
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdint.h"

#include "pb_parser.h"
#include "tx_parser.h"

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a < _b ? _a : _b; })

uint64_t 
decode_varint(const uint8_t *buf, uint8_t *skip_bytes, uint8_t max_len) {
	uint64_t result = 0;
	uint64_t val;
	uint8_t idx = 0;

	while(idx < max_len)
	{
		val = buf[idx] & 0x7f;
		result |= (val << (idx*7));

		// no more bytes
		if(!(buf[idx] & 0x80)) break;

		idx++;
	}

	(*skip_bytes) = idx+1;
	return result;
}

char *
u642str(uint64_t num, char *str, size_t max_len) {

    char temp;
    int last = 0;
    char *start = str, *end = str;

    if (0 == num) {
        str[0] = '0';
        str[1] = 0;
        return str;
    }

    while (num != 0) {
        if (end - start < max_len - 1) {
            last = num % 10;
            *end = last + '0';
            num /= 10;
            end++;
        }
        else {
            /* buffer too short */
            return NULL;
        }
    }

    /* string ends with \0 */
    *end = 0;

    while (start < --end) {
        temp = *start;
        *start = *end;
        *end = temp;
        start++;
    }

    return str;
}

int
decode_tx_pb(const uint8_t *pb_data, uint8_t *skip_bytes_out,uint32_t len, uint32_t *totalfields, int queryid)
{
    uint8_t wire_type;
    uint8_t field_number;
    uint64_t i;
    uint8_t skip_bytes;
    uint32_t total;
    int curid;

    total = 0;
    curid = 0;
    i = 0;
    while (i < len)
    {
        wire_type = pb_data[i] & PB_WIRETYPE_MASK;
        field_number = (pb_data[i] & PB_FIELDNUM_MASK)>>3;
        switch (field_number)
        {
            case ACT_TX_AMOUNT:
                if (wire_type!=PB_WT_LD) return -1; // type doesn't match
                if (i + 1 >= len) return -1;	// overflow
                i++;
                total++;

                int amount_str_len;
                amount_str_len = decode_varint(&pb_data[i], &skip_bytes, len - i);
                i += skip_bytes;

                if (curid == queryid) {
                    int cpylen;
                    cpylen = min(amount_str_len, tx_ctx.query.out_val_len-1);
                    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len,
                         "Tx Amount");
                    strncpy(tx_ctx.query.out_val,(const char *)&pb_data[i],cpylen);
                    tx_ctx.query.out_val[cpylen] = 0;
                }
                i += amount_str_len;
                curid++;
                break;
            case ACT_TX_RECIPIENT:
                if (wire_type!=PB_WT_LD) return -1; // type doesn't match
                if (i + 1 >= len) return -1;	// overflow
                i++;
                total++;

                int recipent_add_len;
                recipent_add_len = decode_varint(&pb_data[i], &skip_bytes, len - i);
                i += skip_bytes;

                if (curid == queryid) {
                    int cpylen;
                    cpylen = min(recipent_add_len, tx_ctx.query.out_val_len-1);
                    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len,
                         "Recipient");
                    strncpy(tx_ctx.query.out_val,(const char *)&pb_data[i],cpylen);
                    tx_ctx.query.out_val[cpylen] = 0;
                }
                i += recipent_add_len;
                curid++;
                break;
            case ACT_TX_PAYLOAD:
                if (wire_type!=PB_WT_LD) return -1; // type doesn't match
                if (i + 1 >= len) return -1;	// overflow
                i++;
                total++;

                int payload_len;
                payload_len = decode_varint(&pb_data[i], &skip_bytes, len - i);
                i += skip_bytes;

                if (curid == queryid) {
                    int printlen;
                    printlen = min(payload_len, (tx_ctx.query.out_val_len)/2 -1);
                    printlen = min(printlen, MAX_PAYLOAD_DISPLAY);
                    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len,
                         "Payload (%d Bytes)", payload_len);
                    for (int offset = 0; offset < printlen; offset++)
                        snprintf(tx_ctx.query.out_val + offset*2, tx_ctx.query.out_val_len-offset*2, "%02X", pb_data[i+offset]);

                    if (payload_len > printlen) {
                        snprintf(tx_ctx.query.out_val + printlen * 2-3, tx_ctx.query.out_val_len-printlen * 2+3, "...");
                    }
                    tx_ctx.query.out_val[printlen * 2] = 0;
                }
                i += payload_len;
                curid++;
                break;

            default:
                return -1;
        }
    }
    (*skip_bytes_out) = i;
    if (totalfields) (*totalfields) = total;
    return 1;
}

int
decode_exe_pb(const uint8_t *pb_data, uint8_t *skip_bytes_out,uint32_t len, uint32_t *totalfields, int queryid)
{
    uint8_t wire_type;
    uint8_t field_number;
    uint64_t i;
    uint8_t skip_bytes;
    uint32_t total;
    int curid;

    total = 0;
    curid = 0;
    i = 0;
    while (i < len)
    {
        wire_type = pb_data[i] & PB_WIRETYPE_MASK;
        field_number = (pb_data[i] & PB_FIELDNUM_MASK)>>3;
        switch (field_number)
        {
            case ACT_EXE_AMOUNT:
                if (wire_type!=PB_WT_LD) return -1; // type doesn't match
                if (i + 1 >= len) return -1;	// overflow
                i++;
                total++;

                int amount_str_len;
                amount_str_len = decode_varint(&pb_data[i], &skip_bytes, len - i);
                i += skip_bytes;

                if (curid == queryid) {
                    int cpylen;
                    cpylen = min(amount_str_len, tx_ctx.query.out_val_len-1);
                    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len,
                        "Exe Amount");
                    strncpy(tx_ctx.query.out_val,(const char *)&pb_data[i],cpylen);
                    tx_ctx.query.out_val[cpylen] = 0;
                }
                i += amount_str_len;
                curid++;
                break;
            case ACT_EXE_CONTRACT:
                if (wire_type!=PB_WT_LD) return -1; // type doesn't match
                if (i + 1 >= len) return -1;	// overflow
                i++;
                total++;

                int contract_add_len;
                contract_add_len = decode_varint(&pb_data[i], &skip_bytes, len - i);
                i += skip_bytes;
                
                if (curid == queryid) {
                    int cpylen;
                    cpylen = min(contract_add_len, tx_ctx.query.out_val_len-1);
                    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len,
                         "Recipient");
                    strncpy(tx_ctx.query.out_val,(const char *)&pb_data[i],cpylen);
                    tx_ctx.query.out_val[cpylen] = 0;
                }
                i += contract_add_len;
                curid++;
                break;
            case ACT_EXE_DATA:
                if (wire_type!=PB_WT_LD) return -1; // type doesn't match
                if (i + 1 >= len) return -1;	// overflow
                i++;
                total++;

                uint32_t datalen;
                datalen = decode_varint(&pb_data[i], &skip_bytes, len - i);
                i += skip_bytes;
                i+= datalen;
                break;

            default:
                return -1;
        }
    }
    (*skip_bytes_out) = i;
    if (totalfields) (*totalfields) = total;
    return 1;
}


int
decode_pb(const uint8_t *pb_data, uint32_t len, uint32_t *totalfields_out, int queryid)
{
    uint8_t wire_type;
    uint8_t field_number;
    uint64_t i;
    uint8_t skip_bytes;
    uint32_t totalfields;
    uint32_t subtotalfields;
    int curid;

    totalfields = 0;
    curid = 0;
    i = 0;
    while (i < len)
    {
        wire_type = pb_data[i] & PB_WIRETYPE_MASK;
        field_number = (pb_data[i] & PB_FIELDNUM_MASK)>>3;
        switch (field_number)
        {
            case ACT_VERSION:
                if (wire_type!=PB_WT_VI) return -1; // type doesn't match
                if (i + 1 >= len) return -1;	// overflow
                i++;
                totalfields++;

                uint32_t version;
                version = decode_varint(&pb_data[i], &skip_bytes, len - i);
                i += skip_bytes;

                if (curid == queryid) {
                    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len,
                         "Version");
                    snprintf(tx_ctx.query.out_val, tx_ctx.query.out_val_len,
                         "%d",version);
                }
                curid++;
                break;
            case ACT_NONCE:
                if (wire_type!=PB_WT_VI) return -1; // type doesn't match
                if (i + 1 >= len) return -1;	// overflow
                i++;
                totalfields++;

                uint64_t nonce;
                nonce = decode_varint(&pb_data[i], &skip_bytes, len - i);
                i += skip_bytes;
                
                if (curid == queryid) {
                    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len,
                         "Nonce");
                    u642str(nonce, tx_ctx.query.out_val, tx_ctx.query.out_val_len);
                }
                curid++;
                break;
            case ACT_GASLIMIT:
                if (wire_type!=PB_WT_VI) return -1; // type doesn't match
                if (i + 1 >= len) return -1;	// overflow
                i++;
                totalfields++;

                uint64_t gas_limit;
                gas_limit = decode_varint(&pb_data[i], &skip_bytes, len - i);
                i += skip_bytes;

                if (curid == queryid) {
                    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len,
                         "Gas Limit");
                    u642str(gas_limit, tx_ctx.query.out_val, tx_ctx.query.out_val_len);
                }
                curid++;
                break;
            case ACT_GASPRICE:
                if (wire_type!=PB_WT_LD) return -1; // type doesn't match
                if (i + 1 >= len) return -1;	// overflow
                i++;
                totalfields++;
                
                int gas_str_len = decode_varint(&pb_data[i], &skip_bytes, len - i);;
                i += skip_bytes;

                if (curid == queryid) {
                    int cpylen;
                    cpylen = min(gas_str_len, tx_ctx.query.out_val_len-1);
                    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len,
                         "Gas Price");
                    strncpy(tx_ctx.query.out_val, (const char *)&pb_data[i], cpylen);
                    tx_ctx.query.out_val[cpylen] = 0;
                }
                i += gas_str_len;
                curid++;

                break; 
            case ACT_TRANSFER:
                if (wire_type!=PB_WT_LD) return -1; // type doesn't match
                if (i + 1 >= len) return -1;	// overflow
                tx_ctx.actiontype = 1; //action is Transfer TBD fix 
                i++;

                uint64_t tsf_len = decode_varint(&pb_data[i], &skip_bytes, len - i);
                i += skip_bytes;

                if ( -1 == decode_tx_pb(&pb_data[i],&skip_bytes,tsf_len, &subtotalfields, queryid - curid )) return -1;

                totalfields += subtotalfields;
                curid += subtotalfields;
                i+= tsf_len;
                break;
            case ACT_EXECUTION:
                if (wire_type!=PB_WT_LD) return -1; // type doesn't match
                if (i + 1 >= len) return -1;	// overflow
                tx_ctx.actiontype = 2; //action is Execution TBD fix 
                i++;

                uint64_t exe_len = decode_varint(&pb_data[i], &skip_bytes, len - i);
                i += skip_bytes;

                if ( -1 == decode_exe_pb(&pb_data[i],&skip_bytes,exe_len,&subtotalfields, queryid - curid )) return -1;
                
                totalfields += subtotalfields;
                curid += subtotalfields;
                i+= exe_len;
                break;
            default:
            
                return -1;
        }
    }
    if (totalfields_out) (*totalfields_out) = totalfields;
    return 1;
}
