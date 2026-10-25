/* Reconstruct lost packets with the real nanors code and guard its work buffer. */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rs.h"

int main(void) {
    enum { BYTES = 67, GUARD = 32 };
    unsigned cases = 0;
    for (int count = 2; count <= 8; count++) {
        for (unsigned mask = 1; mask < (1u << count); mask++) {
            size_t size = reed_solomon_bufsize(count, count);
            uint8_t *buffer = malloc(size + GUARD);
            assert(buffer);
            reed_solomon *rs = reed_solomon_new_static(buffer, size, count, count);
            assert(rs);
            memset(buffer + size, 0xa5, GUARD);
            uint8_t original[8][BYTES], storage[16][BYTES], *shards[16], marks[16] = {0};
            for (int i = 0; i < count * 2; i++) {
                shards[i] = storage[i];
                for (int j = 0; j < BYTES; j++) storage[i][j] = (i * 31 + j * 17 + mask) & 255;
            }
            memcpy(original, storage, count * BYTES);
            assert(reed_solomon_encode(rs, shards, count * 2, BYTES) == 0);
            for (int i = 0; i < count; i++) {
                if (mask & (1u << i)) {
                    marks[i] = 1;
                    memset(shards[i], 0, BYTES);
                }
            }
            assert(reed_solomon_decode(rs, shards, marks, count * 2, BYTES) == 0);
            assert(memcmp(storage, original, count * BYTES) == 0);
            for (int i = 0; i < GUARD; i++) assert(buffer[size + i] == 0xa5);
            free(buffer);
            cases++;
        }
    }
    printf("nanors: %u loss patterns reconstructed without overwriting the work buffer\n", cases);
    return 0;
}
