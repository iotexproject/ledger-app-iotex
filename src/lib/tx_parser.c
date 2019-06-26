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

#include <stdio.h>
#include <string.h>
#include "pb_parser.h"
#include "tx_parser.h"
#include "os.h"

#if defined(TARGET_NANOX) || defined(TARGET_NANOS)
#include "os.h"
#define COPYFUNC os_memmove
#else
#define COPYFUNC memcpy
#define __always_inline
#endif

// Global context to save memory / stack space in recursive calls
parsing_context_t parsing_context;
tx_context_t tx_ctx;

// strcat but source does not need to be terminated (a chunk from a bigger string is concatenated)
// dst_max is measured in bytes including the space for NULL termination
// src_size does not include NULL termination
__always_inline void strcat_chunk_s(char *dst, uint16_t dst_max, const char *src_chunk, uint16_t src_chunk_size) {
    *(dst + dst_max - 1) = 0;                 // last character terminates with zero in case we go beyond bounds
    const uint16_t prev_size = strlen(dst);

    uint16_t space_left = dst_max - prev_size - 1;  // -1 because requires termination

    if (src_chunk_size > space_left) {
        src_chunk_size = space_left;
    }

    if (src_chunk_size > 0) {
        // Check bounds
        COPYFUNC(dst + prev_size, src_chunk, src_chunk_size);
        // terminate
        *(dst + prev_size + src_chunk_size) = 0;
    }
}

int16_t tx_traverse(int16_t root_token_index) {
    

    return 0;
}
