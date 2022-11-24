#include "biginteger.h"


/* Get uint64 from bytes(fix length 8 bytes, big-endian) */
uint64_t bigint_bytes2uint64(const uint8_t *bytes) {
    int i;
    uint64_t u64 = 0;

    for (i = 7; i >= 0 && bytes; i--) {
        u64 |= (uint64_t)bytes[7 - i] << (i * 8);
    }

    return u64;
}

/* Get bigint_uint256 form bytes */
bool bigint_bytes2uint256(const uint8_t *bytes, size_t len, bigint_uint256 *u256, bool be) {
    size_t i;
    uint8_t temp[BIGINT_U256_BYTES] = {0};
    const uint8_t *head = bytes, *tail = bytes + len - 1;

    if (!bytes || !u256 || len > BIGINT_U256_BYTES) {
        return false;
    }

    if (be) {
        for (i = BIGINT_U256_BYTES - len; i < BIGINT_U256_BYTES; i++, head++) {
            temp[i] = *head;
        }
    }
    else {
        for (i = 0; i < len; i++, tail--) {
            temp[i] = *tail;
        }
    }

    u256->high.high = bigint_bytes2uint64(temp);
    u256->high.low = bigint_bytes2uint64(temp + 8);

    u256->low.high = bigint_bytes2uint64(temp + 16);
    u256->low.low = bigint_bytes2uint64(temp + 24);

    return true;
}


/* Convert uint64 to decimal digits 0x255 ==> int8_t[5, 5, 2] */
void bigint_u642dd(uint64_t u64, int8_t *d, size_t size, bool last) {
    int i;
    size_t j;

    if (!d && !size) {
        return;
    }

    for (i = 63; i > -1; i--) {
        if ((u64 >> i) & 1) {
            d[0]++;
        }

        if ((last && i > 0) || !last) {
            for (j = 0; j < size; j++) {
                d[j] *= 2;
            }
        }

        for (j = 0; j < size - 1; j++) {
            d[j + 1] += d[j] / 10, d[j] %= 10;
        }
    }
}

/* Convert decimal digits to string (int8_t[5, 5, 2] ==> "255\0") and return string length */
int bigint_dd2str(const int8_t *dd, size_t size, char *str, size_t max) {
    int i, j;

    if (!str || !dd || max <= size) {
        return -1;
    }

    for (i = size - 1; i > 0; i--) {
        if (dd[i] > 0) {
            break;
        }
    }

    for (j = 0; i > -1; i--) {
        str[j++] = '0' + dd[i];
    }

    str[j] = 0;
    return j;
}

int bigint_u642str(uint64_t u64, char *str, size_t size) {
    int8_t dd[BIGINT_U64_MAX_DD] = {0};

    bigint_u642dd(u64, dd, sizeof(dd), true);

    return bigint_dd2str(dd, sizeof(dd), str, size);
}

int bigint_u1282str(bigint_uint128 u128, char *str, size_t size) {
    int8_t dd[BIGINT_U128_MAX_DD] = {0};

    bigint_u642dd(u128.high, dd, sizeof(dd), false);
    bigint_u642dd(u128.low, dd, sizeof(dd), true);

    return bigint_dd2str(dd, sizeof(dd), str, size);
}

int bigint_u2562str(bigint_uint256 u256, char *str, size_t size) {
    int8_t dd[BIGINT_U256_MAX_DD] = {0};

    bigint_u642dd(u256.high.high, dd, sizeof(dd), false);
    bigint_u642dd(u256.high.low, dd, sizeof(dd), false);

    bigint_u642dd(u256.low.high, dd, sizeof(dd), false);
    bigint_u642dd(u256.low.low, dd, sizeof(dd), true);

    return bigint_dd2str(dd, sizeof(dd), str, size);
}

