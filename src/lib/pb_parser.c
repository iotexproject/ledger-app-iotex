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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pb.h"
#include "pb_decode.h"

#include "pb_parser.h"
#include "tx_parser.h"

#include "action.pb.h"

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a < _b ? _a : _b; })

#define PAYLOAD_STR "Payload"

char *u642str(uint64_t num, char *str, size_t max_len) {
    char temp;
    int last = 0;
    char *start = str, *end = str;

    if (0 == num) {
        str[0] = '0';
        str[1] = 0;
        return str;
    }

    while (num != 0) {
        if ((size_t) (end - start) < max_len - 1) {
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
const char *utils_rau2iotx(const char *rau, size_t rau_len, char *iotx, size_t max) {
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

static void display_ld_item(const uint8_t *pb_data, int ld_len, const char *name, field_type_t type) {
    int offset = 0;
    int cpylen = min(ld_len, tx_ctx.query.out_val_len - 1);
    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len, "%s", name);

    switch (type) {
        case String:
            strncpy(tx_ctx.query.out_val, (const char *)pb_data, cpylen);
            tx_ctx.query.out_val[cpylen] = 0;
            break;

        case Bytes:
            cpylen = min(ld_len, tx_ctx.query.out_val_len / 2 - 1);
            cpylen = min(cpylen, MAX_PAYLOAD_DISPLAY);
            snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len, "%s (%d Bytes)", name, ld_len);

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

static void display_vi_item(uint64_t number, const char *name, field_type_t type) {
    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len, "%s", name);

    if (type == Bool) {
        snprintf(tx_ctx.query.out_val, tx_ctx.query.out_val_len, number ? "true" : "false");
    }
    else {
        u642str(number, tx_ctx.query.out_val, tx_ctx.query.out_val_len);
    }
}

static uint32_t display_sub_msg(const field_display_t *display, size_t length, int totalfields, int queryid) {
    size_t i;
    uint32_t count = 0;

    for (i = 0; i < length; i++, display++) {
        if (IS_FIELD_TYPE_VI(display->type)) {
            if (display->data.varint) {
                if (totalfields == queryid) {
                    display_vi_item(display->data.varint, display->key, display->type);
                }

                count++;
                totalfields++;
            }
        }
        else if (IS_FILED_TYPE_LD(display->type)) {
            if (display->data.ld.buf && display->data.ld.len) {
                if (totalfields == queryid) {
                    display_ld_item((const uint8_t *)display->data.ld.buf, display->data.ld.len, display->key, display->type);
                }

                count++;
                totalfields++;
            }
        }
    }

    return count;
}

static uint32_t display_ld_msg(pb_istream_t *stream, int totalfields, int queryid, int start, int size) {
    int i;
    uint32_t count = 0;
    tx_buffer_t *ld = NULL;

    for (i = 0, ld = tx_ctx.buffer + start; i < size && start + i < TX_BUFFER_SIZE; i++, ld++) {
        if (ld->buf && ld->size && ld->key) {
            if (totalfields == queryid) {
                display_ld_item((const uint8_t *)ld->buf, ld->size, ld->key, ld->type);
            }

            count++;
            totalfields++;
            pb_read(stream, NULL, ld->size);
        }
    }

    return count;
}

static bool read_bytes(pb_istream_t *stream, const pb_field_iter_t *field, void **arg) {
    PB_UNUSED(field);
    int idx =  (intptr_t) *arg;

    if (stream->bytes_left && idx < TX_BUFFER_SIZE) {
        tx_ctx.buffer[idx].buf = stream->state;
        tx_ctx.buffer[idx].size = stream->bytes_left;
    }

    return true;
}

static bool submsg_callback(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    PB_UNUSED(field);
    PB_UNUSED(arg);

    if (iotextypes_ActionCore_transfer_tag == field->tag) {
        iotextypes_Transfer *tx = field->pData;
        tx_ctx.actiontype = ACTION_TX;

        tx_ctx.buffer[0].key = "Amount";
        tx_ctx.buffer[0].type = Iotx;

        tx_ctx.buffer[1].key = "Recipient";
        tx_ctx.buffer[1].type = String;

        tx_ctx.buffer[2].key = PAYLOAD_STR;
        tx_ctx.buffer[2].type = Bytes;

        tx->amount.funcs.decode = read_bytes;
        tx->amount.arg = (void *)0;

        tx->recipient.funcs.decode = read_bytes;
        tx->recipient.arg = (void *)1;

        tx->payload.funcs.decode = read_bytes;
        tx->payload.arg = (void *)2;
    }
    else if (iotextypes_ActionCore_execution_tag == field->tag) {
        iotextypes_Execution *exe = field->pData;
        tx_ctx.actiontype = ACTION_EXE;

        tx_ctx.buffer[0].key = "Amount";
        tx_ctx.buffer[0].type = Iotx;

        tx_ctx.buffer[1].key = "Contract";
        tx_ctx.buffer[1].type = String;

        tx_ctx.buffer[2].key = "Data";
        tx_ctx.buffer[2].type = Bytes;

        exe->amount.funcs.decode = read_bytes;
        exe->amount.arg = (void *)0;

        exe->contract.funcs.decode = read_bytes;
        exe->contract.arg = (void *)1;

        exe->data.funcs.decode = read_bytes;
        exe->data.arg = (void *)2;
    }
    else if (iotextypes_ActionCore_stakeCreate_tag == field->tag) {
        iotextypes_StakeCreate *create = field->pData;
        tx_ctx.actiontype = ACTION_SKT_CREATE;

        tx_ctx.buffer[0].key = "Candidate Name";
        tx_ctx.buffer[0].type = String;

        tx_ctx.buffer[1].key = "Staked Amount";
        tx_ctx.buffer[1].type = Iotx;

        tx_ctx.buffer[2].key = PAYLOAD_STR;
        tx_ctx.buffer[2].type = Bytes;

        create->candidateName.funcs.decode = read_bytes;
        create->candidateName.arg = (void *)0;

        create->stakedAmount.funcs.decode = read_bytes;
        create->stakedAmount.arg = (void *)1;

        create->payload.funcs.decode = read_bytes;
        create->payload.arg = (void *)2;
    }
    else if (iotextypes_ActionCore_stakeUnstake_tag == field->tag) {
        iotextypes_StakeReclaim *unstake = field->pData;
        tx_ctx.actiontype = ACTION_SKT_UNSTAKE;

        tx_ctx.buffer[0].key = PAYLOAD_STR;
        tx_ctx.buffer[0].type = Bytes;

        unstake->payload.funcs.decode = read_bytes;
        unstake->payload.arg = (void *)0;
    }
    else if (iotextypes_ActionCore_stakeWithdraw_tag == field->tag) {
        iotextypes_StakeReclaim *withdraw = field->pData;
        tx_ctx.actiontype = ACTION_SKT_WITHDRAW;

        tx_ctx.buffer[0].key = PAYLOAD_STR;
        tx_ctx.buffer[0].type = Bytes;

        withdraw->payload.funcs.decode = read_bytes;
        withdraw->payload.arg = (void *)0;
    }
    else if (iotextypes_ActionCore_stakeAddDeposit_tag == field->tag) {
        iotextypes_StakeAddDeposit *deposit = field->pData;
        tx_ctx.actiontype = ACTION_SKT_ADD_DEPOSIT;

        tx_ctx.buffer[0].key = "Amount";
        tx_ctx.buffer[0].type = Iotx;

        tx_ctx.buffer[1].key = PAYLOAD_STR;
        tx_ctx.buffer[1].type = Bytes;

        deposit->amount.funcs.decode = read_bytes;
        deposit->amount.arg = (void *)0;

        deposit->payload.funcs.decode = read_bytes;
        deposit->payload.arg = (void *)1;
    }
    else if (iotextypes_ActionCore_stakeRestake_tag == field->tag) {
        iotextypes_StakeRestake *restake = field->pData;
        tx_ctx.actiontype = ACTION_SKT_RESTAKE;

        tx_ctx.buffer[0].key = PAYLOAD_STR;
        tx_ctx.buffer[0].type = Bytes;

        restake->payload.funcs.decode = read_bytes;
        restake->payload.arg = (void *)0;
    }
    else if (iotextypes_ActionCore_stakeChangeCandidate_tag == field->tag) {
        iotextypes_StakeChangeCandidate *cdd = field->pData;
        tx_ctx.actiontype = ACTION_SKT_CHANGE_CDD;

        tx_ctx.buffer[0].key = "Candidate Name";
        tx_ctx.buffer[0].type = String;

        tx_ctx.buffer[1].key = PAYLOAD_STR;
        tx_ctx.buffer[1].type = Bytes;

        cdd->candidateName.funcs.decode = read_bytes;
        cdd->candidateName.arg = (void *)0;

        cdd->payload.funcs.decode = read_bytes;
        cdd->payload.arg = (void *)1;
    }
    else if (iotextypes_ActionCore_stakeTransferOwnership_tag == field->tag) {
        iotextypes_StakeTransferOwnership *ownership = field->pData;
        tx_ctx.actiontype = ACTION_SKT_TX_OWNERSHIP;

        tx_ctx.buffer[0].key = "Voter Address";
        tx_ctx.buffer[0].type = String;

        tx_ctx.buffer[1].key = PAYLOAD_STR;
        tx_ctx.buffer[1].type = Bytes;

        ownership->voterAddress.funcs.decode = read_bytes;
        ownership->voterAddress.arg = (void *)0;

        ownership->payload.funcs.decode = read_bytes;
        ownership->payload.arg = (void *)1;
    }
    else if (iotextypes_ActionCore_candidateRegister_tag == field->tag) {
        iotextypes_CandidateRegister *cdd = field->pData;
        tx_ctx.actiontype = ACTION_SKT_CDD_REGISTER;

        tx_ctx.buffer[0].key = "Staked Amount";
        tx_ctx.buffer[0].type = Iotx;

        tx_ctx.buffer[1].key = "Owner Address";
        tx_ctx.buffer[1].type = String;

        tx_ctx.buffer[2].key = PAYLOAD_STR;
        tx_ctx.buffer[2].type = Bytes;

        cdd->stakedAmount.funcs.decode = read_bytes;
        cdd->stakedAmount.arg = (void *)0;

        cdd->ownerAddress.funcs.decode = read_bytes;
        cdd->ownerAddress.arg = (void *)1;

        cdd->payload.funcs.decode = read_bytes;
        cdd->payload.arg = (void *)2;

        tx_ctx.buffer[3].buf = stream->state;
        tx_ctx.buffer[3].size = stream->bytes_left;

        pb_read(stream, NULL, stream->bytes_left);
    }
    else if (iotextypes_ActionCore_candidateUpdate_tag == field->tag) {
        tx_ctx.actiontype = ACTION_SKT_CDD_UPDATE;
    }

    return true;
}

static uint32_t display_transfer(pb_istream_t *stream, const iotextypes_Transfer *tx, int queryid) {
    PB_UNUSED(tx);
    return display_ld_msg(stream, 0, queryid, 0, 3);
}

static uint32_t display_execution(pb_istream_t *stream, const iotextypes_Execution *exe, int queryid) {
    PB_UNUSED(exe);
    return display_ld_msg(stream, 0, queryid, 0, 3);
}

static uint32_t display_stake_create(pb_istream_t *stream, const iotextypes_StakeCreate *create, int queryid) {
    int totalfields = 0;
    const field_display_t members[] = {
        {"Staked Duration", Varint, {create->stakedDuration}},
        {"Auto Stake", Bool, {create->autoStake}},
    };

    /* candidateName/stakedAmount */
    totalfields += display_ld_msg(stream, totalfields, queryid, 0, 2);

    /* stakedDuration/autoStake */
    totalfields += display_sub_msg(members, sizeof(members) / sizeof(members[0]), totalfields, queryid);

    /* payload */
    totalfields += display_ld_msg(stream, totalfields, queryid, 2, 1);

    return totalfields;
}

static uint32_t display_stake_restake(pb_istream_t *stream, const iotextypes_StakeRestake *restake, int queryid) {
    int totalfields = 0;
    const field_display_t members[] = {
        {"Bucket Index", Varint, {restake->bucketIndex}},
        {"Staked Duration", Varint, {restake->stakedDuration}},
        {"Auto Stake", Bool, {restake->autoStake}},
    };

    /* bucketIndex/stakedDuration/autoStake */
    totalfields += display_sub_msg(members, sizeof(members) / sizeof(members[0]), totalfields, queryid);

    /* payload */
    totalfields += display_ld_msg(stream, totalfields, queryid, 0, 1);

    return totalfields;
}

static uint32_t display_stake_reclaim(pb_istream_t *stream, const iotextypes_StakeReclaim *reclaim, int queryid) {
    int totalfields = 0;

    if (reclaim->bucketIndex) {
        if (totalfields == queryid) {
            display_vi_item(reclaim->bucketIndex, "Bucket Index", Varint);
        }

        totalfields++;
    }

    /* Payload */
    totalfields += display_ld_msg(stream, totalfields, queryid, 0, 1);

    return totalfields;
}

static uint32_t display_stake_add_deposit(pb_istream_t *stream, const iotextypes_StakeAddDeposit *deposit, int queryid) {
    int totalfields = 0;

    if (deposit->bucketIndex) {
        if (totalfields == queryid) {
            display_vi_item(deposit->bucketIndex, "Bucket Index", Varint);
        }

        totalfields++;
    }

    /* amount/payload */
    totalfields += display_ld_msg(stream, totalfields, queryid, 0, 2);

    return totalfields;
}

static uint32_t display_stake_change_cdd(pb_istream_t *stream, const iotextypes_StakeChangeCandidate *cdd, int queryid) {
    int totalfields = 0;

    if (cdd->bucketIndex) {
        if (totalfields == queryid) {
            display_vi_item(cdd->bucketIndex, "Bucket Index", Varint);
        }

        totalfields++;
    }

    /* candidateName/payload */
    totalfields += display_ld_msg(stream, totalfields, queryid, 0, 2);

    return totalfields;
}

static uint32_t display_stake_cdd_update(pb_istream_t *stream, const iotextypes_CandidateBasicInfo *cdd, int queryid) {
    PB_UNUSED(stream);
    const field_display_t members[] = {
        {"Name", String, {.ld = {(const char *)cdd->name, strlen(cdd->name)}}},
        {"Operator Address", String, {.ld = {(const char *)cdd->operatorAddress, strlen(cdd->operatorAddress)}}},
        {"Reward Address", String, {.ld = {(const char *)cdd->rewardAddress, strlen(cdd->rewardAddress)}}},
    };

    return display_sub_msg(members, sizeof(members) / sizeof(members[0]), 0, queryid);
}

static uint32_t display_stake_cdd_register(pb_istream_t *stream, iotextypes_CandidateRegister *cdd, int queryid) {
    int totalfields = 0;
    pb_istream_t sub_stream = pb_istream_from_buffer(tx_ctx.buffer[3].buf, tx_ctx.buffer[3].size);

    if (!pb_decode(&sub_stream, iotextypes_CandidateRegister_fields, cdd)) {
        return 0;
    }

    if (cdd->has_candidate) {
        totalfields += display_stake_cdd_update(stream, &cdd->candidate, queryid);
    }

    const field_display_t members[] = {
        {"Staked Duration", Varint, {cdd->stakedDuration}},
        {"Auto Stake", Bool, {cdd->autoStake}},
    };

    /* stakedAmount */
    totalfields += display_ld_msg(stream, totalfields, queryid, 0, 1);

    /* stakedDuration/autoStake */
    totalfields += display_sub_msg(members, sizeof(members) / sizeof(members[0]), totalfields, queryid);

    /* ownerAddress/payload */
    totalfields += display_ld_msg(stream, totalfields, queryid, 1, 2);

    return totalfields;
}

static uint32_t display_stake_tx_ownership(pb_istream_t *stream, const iotextypes_StakeTransferOwnership *ownership, int queryid) {
    int totalfields = 0;

    if (ownership->bucketIndex) {
        if (totalfields == queryid) {
            display_vi_item(ownership->bucketIndex, "Bucket Index", Varint);
        }

        totalfields++;
    }

    /* voterAddress/payload */
    totalfields += display_ld_msg(stream, totalfields, queryid, 0, 2);

    return totalfields;
}

int decode_pb(const uint8_t *pb_data, uint32_t len, uint32_t *totalfields_out, int queryid) {
    int totalfields = 0;
    pb_istream_t istream = pb_istream_from_buffer(pb_data, len);
    iotextypes_ActionCore action_core = iotextypes_ActionCore_init_zero;

    memset(&tx_ctx.buffer, 0, sizeof(tx_ctx.buffer));
    action_core.cb_action.funcs.decode = submsg_callback;

    if (!pb_decode(&istream, iotextypes_ActionCore_fields, &action_core)) {
        return -1;
    }

    if (action_core.version) {
        if (totalfields == queryid) {
            display_vi_item(action_core.version, "Version", Varint);
        }

        totalfields++;
    }

    if (action_core.nonce) {
        if (totalfields == queryid) {
            display_vi_item(action_core.nonce, "Nonce", Varint);
        }

        totalfields++;
    }

    if (action_core.gasLimit) {
        if (totalfields == queryid) {
            display_vi_item(action_core.gasLimit, "Gas Limit", Varint);
        }

        totalfields++;
    }

    if (strlen(action_core.gasPrice)) {
        if (totalfields == queryid) {
            display_ld_item((const uint8_t *)action_core.gasPrice, strlen(action_core.gasPrice), "Gas Price", String);
        }

        totalfields++;
    }

    switch (action_core.which_action) {
        case iotextypes_ActionCore_transfer_tag:
            totalfields += display_transfer(&istream, &action_core.action.transfer, queryid - totalfields);
            break;

        case iotextypes_ActionCore_execution_tag:
            totalfields += display_execution(&istream, &action_core.action.execution, queryid - totalfields);
            break;

        case iotextypes_ActionCore_stakeCreate_tag:
            totalfields += display_stake_create(&istream, &action_core.action.stakeCreate, queryid - totalfields);
            break;

        case iotextypes_ActionCore_stakeUnstake_tag:
            totalfields += display_stake_reclaim(&istream, &action_core.action.stakeUnstake, queryid - totalfields);
            break;

        case iotextypes_ActionCore_stakeWithdraw_tag:
            totalfields += display_stake_reclaim(&istream, &action_core.action.stakeWithdraw, queryid - totalfields);
            break;

        case iotextypes_ActionCore_stakeAddDeposit_tag:
            totalfields += display_stake_add_deposit(&istream, &action_core.action.stakeAddDeposit, queryid - totalfields);
            break;

        case iotextypes_ActionCore_stakeRestake_tag:
            totalfields += display_stake_restake(&istream, &action_core.action.stakeRestake, queryid - totalfields);
            break;

        case iotextypes_ActionCore_stakeChangeCandidate_tag:
            totalfields += display_stake_change_cdd(&istream, &action_core.action.stakeChangeCandidate, queryid - totalfields);
            break;

        case iotextypes_ActionCore_stakeTransferOwnership_tag:
            totalfields += display_stake_tx_ownership(&istream, &action_core.action.stakeTransferOwnership, queryid - totalfields);
            break;

        case iotextypes_ActionCore_candidateRegister_tag:
            totalfields += display_stake_cdd_register(&istream, &action_core.action.candidateRegister, queryid - totalfields);
            break;

        case iotextypes_ActionCore_candidateUpdate_tag:
            totalfields += display_stake_cdd_update(&istream, &action_core.action.candidateUpdate, queryid - totalfields);
            break;

        default:
            return  -2;
    }

    if (totalfields_out) {
        *totalfields_out = totalfields;
    }

    return 0;
}

