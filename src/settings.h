#ifndef _NVM_SETTINGS_H_
#define _NVM_SETTINGS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct nvm_settings_t {
    unsigned char contractDataAllowed;
    unsigned char pad[2];
    uint8_t initialized;
} nvm_settings_t;

extern const nvm_settings_t N_nvm_settings;
#define N_settings (*(volatile nvm_settings_t *) PIC(&N_nvm_settings))


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif




