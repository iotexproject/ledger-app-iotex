#ifndef _BIT_INTERGER_H_
#define _BIT_INTERGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BIGINT_U256_BYTES 32
#define BIGINT_U64_MAX_DD 20
#define BIGINT_U128_MAX_DD 39
#define BIGINT_U256_MAX_DD 78


typedef struct {
    uint64_t high, low;
} bigint_uint128;

typedef struct {
    bigint_uint128 high, low;
} bigint_uint256;

uint64_t bigint_bytes2uint64(const uint8_t *bytes);
bool bigint_bytes2uint256(const uint8_t *bytes, size_t len, bigint_uint256 *u256, bool be);

void bigint_u642dd(uint64_t u64, int8_t *d, size_t size, bool last);
int bigint_dd2str(const int8_t *dd, size_t size, char *str, size_t max);

int bigint_u642str(uint64_t u64, char *str, size_t size);
int bigint_u1282str(bigint_uint128 u128, char *str, size_t size);
int bigint_u2562str(bigint_uint256 u256, char *str, size_t size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif




