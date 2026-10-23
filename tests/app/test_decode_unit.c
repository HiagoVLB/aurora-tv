#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stream/video/decode_unit.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    unsigned char data[] = {1,2,3,4,5,6,7,8};
    unsigned char output[8];
    LENTRY b = {.data=(char *) data+4, .length=4};
    LENTRY a = {.data=(char *) data, .length=4, .next=&b};
    DECODE_UNIT unit = {.fullLength=8, .bufferList=&a};
    assert(video_decode_unit_copy(output, sizeof(output), &unit));
    assert(memcmp(output, data, 8) == 0);
    assert(!video_decode_unit_copy(output, 7, &unit));
    for (int n = -2; n <= 10; n++) {
        unit.fullLength = n;
        assert(video_decode_unit_copy(output, sizeof(output), &unit) == (n == 8));
    }
    unit.fullLength=8;
    b.length=INT_MAX; assert(!video_decode_unit_copy(output, sizeof(output), &unit));
    b.length=-1; assert(!video_decode_unit_copy(output, sizeof(output), &unit));
    b.length=0; assert(!video_decode_unit_copy(output, sizeof(output), &unit));
    b.length=4; b.data=NULL; assert(!video_decode_unit_copy(output, sizeof(output), &unit));
    b.data=(char *) data+4; b.next=&a; assert(!video_decode_unit_copy(output, sizeof(output), &unit));
    b.next=NULL; a.next=NULL; unit.fullLength=4;
    assert(video_decode_unit_copy(output, sizeof(output), &unit));
    unit.bufferList=NULL; assert(!video_decode_unit_copy(output, sizeof(output), &unit));
    assert(!video_decode_unit_copy(output, sizeof(output), NULL));
    assert(!video_decode_unit_copy(NULL, sizeof(output), &unit));
    puts("Decode unit: valid frames, capacity, mismatched/negative/overflow lengths, nulls and cycles passed");
}
