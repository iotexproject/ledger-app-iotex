#include "os.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tx_parser.h"
#include "pb_parser.h"

void *pic(void *linked_address) {
    return linked_address;
}

#define MAX_CHARS_TITLE             32
#define MAX_CHARS_PER_KEY_LINE      64
#define MAX_CHARS_PER_VALUE_LINE    192
#define MAX_SCREEN_LINE_WIDTH       19

typedef struct {
    // Index of the currently displayed page
    int16_t detailsCurrentPage;
    // Total number of displayable pages
    int16_t detailsPageCount;

    // When data goes beyond the limit, it will be split in chunks that
    // that spread over several pages
    // Index of currently displayed value chunk
    int16_t chunksIndex;
    // Total number of displayable value chunks
    int16_t chunksCount;

    // DATA
    char title[MAX_SCREEN_LINE_WIDTH];
    char dataKey[MAX_CHARS_PER_KEY_LINE];
    char dataValue[MAX_CHARS_PER_VALUE_LINE];

#if defined(TARGET_NANOX)
    char dataValueChunk[MAX_SCREEN_NUM_LINES][MAX_SCREEN_LINE_WIDTH+1];
#endif
} viewctl_s;

#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    viewctl_s viewctl = {0};

    // Read key and value strings
    INIT_QUERY(
        viewctl.dataKey, sizeof(viewctl.dataKey),
        viewctl.dataValue, sizeof(viewctl.dataValue),
        viewctl.chunksIndex);

    int error_code = decode_pb(Data, Size, NULL, -1);
    if (error_code != 0) {
        return 0;
    }
    
    uint32_t total_fields = 0;
    decode_pb(Data, Size, &total_fields, 0);
    for (int i = 0; i < total_fields; i++) {
        decode_pb(Data, Size, &total_fields, i);
        // printf("%s %s\n", viewctl.dataKey, viewctl.dataValue);
    }

    return 0;
}
