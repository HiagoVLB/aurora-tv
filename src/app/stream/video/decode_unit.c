#include "decode_unit.h"

#include <string.h>

bool video_decode_unit_copy(unsigned char *buffer, size_t capacity, const DECODE_UNIT *unit) {
    if (buffer == NULL || unit == NULL || unit->fullLength <= 0 ||
        (size_t) unit->fullLength > capacity || unit->bufferList == NULL) {
        return false;
    }
    const size_t expected = (size_t) unit->fullLength;
    size_t length = 0;
    for (PLENTRY entry = unit->bufferList; entry != NULL; entry = entry->next) {
        /* Subtract before adding: also bounds cycles and avoids integer wrap.
         * Check against fullLength even when the allocation has spare capacity. */
        if (entry->data == NULL || entry->length <= 0 ||
            (size_t) entry->length > expected - length) {
            return false;
        }
        memcpy(buffer + length, entry->data, (size_t) entry->length);
        length += (size_t) entry->length;
    }
    return length == expected;
}
