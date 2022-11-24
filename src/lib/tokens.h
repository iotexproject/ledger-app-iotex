#ifndef _TOKENS_H_
#define _TOKENS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

#define IOTEX_TOKEN_COUNT 27
#define TOKEN_ADDRESS_LENGTH 20
#define TOKEN_MAX_TICKER_LEN 12
#define TOKEN_BECH32_ADDR_LEN 42

typedef struct {
    uint8_t address[TOKEN_ADDRESS_LENGTH];
    uint8_t ticker[TOKEN_MAX_TICKER_LEN];
    uint8_t decimals;
} token_t;

extern const token_t iotex_tokens[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
