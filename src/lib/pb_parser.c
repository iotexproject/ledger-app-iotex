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

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

uint64_t
decode_varint(const uint8_t *buf, uint8_t *skip_bytes, uint8_t max_len) {
    uint64_t result = 0;
    uint64_t val;
    uint8_t idx = 0;

    while (idx < max_len)
    {
        val = buf[idx] & 0x7f;
        result |= (val << (idx * 7));

        // no more bytes
        if (!(buf[idx] & 0x80)) break;

        idx++;
    }

    (*skip_bytes) = idx + 1;
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

/* Transform rau to iotx: 1 iotx = 10**18 rau */
const char *
utils_rau2iotx(const char *rau, size_t rau_len, char *iotx, size_t max) {
    size_t r, w;
    size_t decimal_point_pos, pad_zero;
    static const size_t transform_factor = 18;

    /* iotx buffer too short or empty */
    if (max < rau_len + 3 || max < transform_factor + 3 || !iotx) {
        return NULL;
    }

    if ('0' == rau[0]) {
        iotx[0] = '0';
        iotx[1] = 0;
        return iotx;
    }

    if (rau_len >= transform_factor) {
        decimal_point_pos = rau_len - transform_factor;

        for (r = 0, w = 0; r < rau_len;) {
            iotx[w++] = rau[r++];

            if (r == decimal_point_pos) {
                iotx[w++] = '.';
            }
        }
    }
    else {
        r = w = 0;
        iotx[w++] = '0';
        iotx[w++] = '.';
        pad_zero = transform_factor - rau_len;

        do {
            iotx[w++] = '0';
        } while (w < pad_zero + 2);

        do {
            iotx[w++] = rau[r++];
        } while (r < rau_len);

    }

    /* Ending */
    w -= 1;

    /* Remove useless zeros */
    while ('0' == iotx[w]) {
        --w;
    }

    /* Round number */
    if ('.' == iotx[w]) {
        --w;
    }

    iotx[w + 1] = 0;
    return iotx;
}

#define KEY_MAX_LEN 20

enum field_type {
    EmbMsg = 0x1F00,
    String = 0x1200,
    Bytes = 0x1300,
    Iotx = 0x1400,
    Varint = 0x2000,
    Bool = 0x2100,
};

struct pb_field {
    const char key[KEY_MAX_LEN];
    enum field_type type;
};

#define IS_FILED_TYPE_LD(x) ((x) & 0x1000)
#define IS_FIELD_TYPE_VI(x) ((x) & 0x2000)
#define IS_FIELE_TYPE_EMB(x) (((x) & EmbMsg) == EmbMsg)

#define GET_EMBMSG_FIELD(x) ((x) & 0xff)
#define SET_EMBMSG_FIELD(x) (enum field_type)((((x) & 0xff) | EmbMsg))

static void display_ld_item(const uint8_t *pb_data, int ld_len, const struct pb_field *field) {
    int offset = 0;
    int cpylen = min(ld_len, tx_ctx.query.out_val_len - 1);
    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len, "%s", field->key);

    switch (field->type) {
        case String:
            strncpy(tx_ctx.query.out_val, (const char *)pb_data, cpylen);
            tx_ctx.query.out_val[cpylen] = 0;
            break;

        case Bytes:
            cpylen = min(ld_len, tx_ctx.query.out_val_len / 2 - 1);
            cpylen = min(cpylen, MAX_PAYLOAD_DISPLAY);
            snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len, "Payload (%d Bytes)", ld_len);

            for (offset = 0; offset < cpylen; offset++) {
                snprintf(tx_ctx.query.out_val + offset * 2, tx_ctx.query.out_val_len - offset * 2, "%02X", pb_data[offset]);
            }

            /* Too much */
            if (ld_len > cpylen) {
                snprintf(tx_ctx.query.out_val + cpylen * 2 - 3, tx_ctx.query.out_val_len - cpylen * 2 + 3, "...");
            }

            tx_ctx.query.out_val[cpylen * 2] = 0;
            break;

        case Iotx:
            utils_rau2iotx((const char *)pb_data, cpylen, tx_ctx.query.out_val, tx_ctx.query.out_val_len);
            break;

        default:
            break;
    }
}

static void display_vi_item(uint64_t number, const struct pb_field *field) {
    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len, "%s", field->key);

    if (field->type == Bool) {
        snprintf(tx_ctx.query.out_val, tx_ctx.query.out_val_len, number ? "true" : "false");
    }
    else {
        u642str(number, tx_ctx.query.out_val, tx_ctx.query.out_val_len);
    }
}


static int
decode_action(const uint8_t *pb_data, uint8_t *skip_bytes_out, uint32_t len, uint32_t *totalfields, int queryid, int action_id) {
    static const struct pb_field create_field[] = {
        {"Candidate Name", String},
        {"Staked Amount", Iotx},
        {"Staked Duration", Varint},
        {"Auto Stake", Bool},
        {"Payload", Bytes}
    };

    static const struct pb_field reclaim_field[] = {
        {"Bucket Index", Varint},
        {"Payload", Bytes},
    };

    static const struct pb_field add_deposit_field[] = {
        {"Bucket Index", Varint},
        {"Amount", Iotx},
        {"Payload", Bytes},
    };

    static const struct pb_field restake_field[] = {
        {"Bucket Index", Varint},
        {"Staked Duration", Varint},
        {"Auto Stake", Bool},
        {"Payload", Bytes}
    };

    static const struct pb_field change_candidate_field[] = {
        {"Bucket Index", Varint},
        {"Candidate Name", String},
        {"Payload", Bytes}
    };

    static const struct pb_field tx_ownership_field[] = {
        {"Bucket Index", Varint},
        {"Voter Address", String},
        {"Payload", Bytes}
    };

    static const struct pb_field candidate_info_field[] = {
        {"Name", String},
        {"Operator Address", String},
        {"Reward Address", String}
    };

    static const struct pb_field candidate_register_field[] = {
        {"Info", SET_EMBMSG_FIELD(ACT_STAKE_CDD_UPDATE)},
        {"Staked Amount", Iotx},
        {"Staked Duration", Varint},
        {"Auto Stake", Bool},
        {"Owner Address", String},
        {"Payload", Bytes}
    };

    static const struct pb_field transfer_field[] = {
        {"Amount", Iotx},
        {"Recipient", String},
        {"Payload", Bytes},
    };

    static const struct pb_field execution_field[] = {
        {"Amount", Iotx},
        {"Contract ", String},
        {"Data", Bytes},
    };

    uint32_t i = 0;
    uint32_t total = 0;
    int current_id = 0;

    uint8_t wire_type;
    uint8_t skip_bytes;
    uint64_t header = 0;
    uint8_t field_count = 0;
    uint8_t field_number = 0;
    const struct pb_field *action_field = NULL;
    const struct pb_field *current_field = NULL;

    switch (action_id) {
        case ACT_TRANSFER:
            action_field = transfer_field;
            field_count = ARRAY_SIZE(transfer_field);
            break;

        case ACT_EXECUTION:
            action_field = execution_field;
            field_count = ARRAY_SIZE(execution_field);
            break;

        case ACT_STAKE_CREATE:
            action_field = create_field;
            field_count = ARRAY_SIZE(create_field);
            break;

        case ACT_STAKE_UNSTAKE:
        case ACT_STAKE_WITHDRAW:
            action_field = reclaim_field;
            field_count = ARRAY_SIZE(reclaim_field);
            break;

        case ACT_STAKE_ADD_DEPOSIT:
            action_field = add_deposit_field;
            field_count = ARRAY_SIZE(add_deposit_field);
            break;

        case ACT_STAKE_RESTAKE:
            action_field = restake_field;
            field_count = ARRAY_SIZE(restake_field);
            break;

        case ACT_STAKE_CHANGE_CDD:
            action_field = change_candidate_field;
            field_count = ARRAY_SIZE(change_candidate_field);
            break;

        case ACT_STAKE_TX_OWNERSHIP:
            action_field = tx_ownership_field;
            field_count = ARRAY_SIZE(tx_ownership_field);
            break;

        case ACT_STAKE_CDD_REGISTER:
            action_field = candidate_register_field;
            field_count = ARRAY_SIZE(candidate_register_field);
            break;

        case ACT_STAKE_CDD_UPDATE:
            action_field = candidate_info_field;
            field_count = ARRAY_SIZE(candidate_info_field);
            break;

        default:
            return -2;
    }

    while (i < len) {
        header = decode_varint(pb_data + i, &skip_bytes, len - i);
        wire_type = PB_GET_WTYPE(header);
        field_number = PB_GET_FIELD(header);
        i += skip_bytes;

        /* Invalid field number */
        if (!field_number || field_number > field_count) {
            return -3;
        }

        /* Overflow */
        if (i + 1 >= len) {
            return -4;
        }

        /* Current field */
        total++;
        current_field = action_field + field_number - 1;

        /* Decode a varint for ld msg is msg length, for varint is field value */
        uint64_t number = decode_varint(&pb_data[i], &skip_bytes, len - i);
        i += skip_bytes;

        if (IS_FILED_TYPE_LD(current_field->type) && (PB_WT_LD == wire_type)) {
            if (IS_FIELE_TYPE_EMB(current_field->type)) {
                uint8_t sub_skip_bytes = 0;
                uint32_t sub_totalfields = 0;

                /* Embedded filed */
                if (decode_action(pb_data + i, &sub_skip_bytes, number,
                                  &sub_totalfields, queryid - current_id,
                                  GET_EMBMSG_FIELD(current_field->type)) == -1) {
                    return -5;
                }

                i += number;
                total += sub_totalfields;
                current_id += sub_totalfields;
                continue;
            }

            /* String/Bytes/Iotx */
            if (current_id == queryid) {
                display_ld_item(pb_data + i, number, current_field);
            }

            i += number;
            current_id++;
        }
        else if (IS_FIELD_TYPE_VI(current_field->type) && (PB_WT_VI == wire_type)) {
            if (current_id == queryid) {
                display_vi_item(number, current_field);
            }

            current_id++;
        }
        else {
            return -6;
        }

    }

    if (skip_bytes_out) {
        *skip_bytes_out = i;
    }

    if (totalfields) {
        *totalfields = total;
    }

    return 0;
}

int
decode_pb(const uint8_t *pb_data, uint32_t len, uint32_t *totalfields_out, int queryid)
{
    uint64_t header;
    uint8_t wire_type;
    uint64_t field_number;

    uint64_t i;
    uint8_t skip_bytes;
    uint32_t totalfields;
    uint32_t subtotalfields;
    int curid;

    totalfields = 0;
    curid = 0;
    i = 0;
    int ret;

    while (i < len) {
        header = decode_varint(&pb_data[i], &skip_bytes, len - i);
        wire_type = PB_GET_WTYPE(header);
        field_number = PB_GET_FIELD(header);
        i += skip_bytes;


        switch (field_number) {
            case ACT_VERSION:
                if (wire_type != PB_WT_VI) return -1; // type doesn't match

                if (i + 1 >= len) return -1;	// overflow

                totalfields++;

                uint32_t version;
                version = decode_varint(&pb_data[i], &skip_bytes, len - i);
                i += skip_bytes;

                if (curid == queryid) {
                    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len, "Version");
                    snprintf(tx_ctx.query.out_val, tx_ctx.query.out_val_len, "%d", version);
                }

                curid++;
                break;

            case ACT_NONCE:
                if (wire_type != PB_WT_VI) return -1; // type doesn't match

                if (i + 1 >= len) return -1;	// overflow

                totalfields++;

                uint64_t nonce;
                nonce = decode_varint(&pb_data[i], &skip_bytes, len - i);
                i += skip_bytes;

                if (curid == queryid) {
                    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len, "Nonce");
                    u642str(nonce, tx_ctx.query.out_val, tx_ctx.query.out_val_len);
                }

                curid++;
                break;

            case ACT_GASLIMIT:
                if (wire_type != PB_WT_VI) return -1; // type doesn't match

                if (i + 1 >= len) return -1;	// overflow

                totalfields++;

                uint64_t gas_limit;
                gas_limit = decode_varint(&pb_data[i], &skip_bytes, len - i);
                i += skip_bytes;

                if (curid == queryid) {
                    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len, "Gas Limit");
                    u642str(gas_limit, tx_ctx.query.out_val, tx_ctx.query.out_val_len);
                }

                curid++;
                break;

            case ACT_GASPRICE:
                if (wire_type != PB_WT_LD) return -1; // type doesn't match

                if (i + 1 >= len) return -1;	// overflow

                totalfields++;

                int gas_str_len = decode_varint(&pb_data[i], &skip_bytes, len - i);;
                i += skip_bytes;

                if (curid == queryid) {
                    int cpylen;
                    cpylen = min(gas_str_len, tx_ctx.query.out_val_len - 1);
                    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len, "Gas Price");
                    strncpy(tx_ctx.query.out_val, (const char *)&pb_data[i], cpylen);
                    tx_ctx.query.out_val[cpylen] = 0;
                }

                i += gas_str_len;
                curid++;

                break;

            case ACT_TRANSFER:
            case ACT_EXECUTION:
            case ACT_STAKE_CREATE:
            case ACT_STAKE_UNSTAKE:
            case ACT_STAKE_WITHDRAW:
            case ACT_STAKE_ADD_DEPOSIT:
            case ACT_STAKE_RESTAKE:
            case ACT_STAKE_CHANGE_CDD:
            case ACT_STAKE_TX_OWNERSHIP:
            case ACT_STAKE_CDD_REGISTER:
            case ACT_STAKE_CDD_UPDATE:
                if ((wire_type != PB_WT_LD) || (i + 1 >= len)) {
                    return -1;
                }

                /* Action type from 1 to ACT_MAX_INVALID */
                if (ACT_EXECUTION == field_number) {
                    tx_ctx.actiontype = ACTION_EXE;
                }
                else if (ACT_TRANSFER == field_number) {
                    tx_ctx.actiontype = ACTION_TX;
                }
                else {
                    tx_ctx.actiontype = field_number - ACT_STAKE_CREATE + 3;
                }

                uint64_t msg_len = decode_varint(&pb_data[i], &skip_bytes, len - i);
                i += skip_bytes;

                ret = decode_action(&pb_data[i], &skip_bytes, msg_len, &subtotalfields, queryid - curid, field_number);

                if (ret != 0) {
                    return ret;
                }

                totalfields += subtotalfields;
                curid += subtotalfields;
                i += msg_len;
                break;

            default:
                return -field_number;
        }
    }

    if (totalfields_out) (*totalfields_out) = totalfields;

    return 0;
}
