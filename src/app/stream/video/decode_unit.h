#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <Limelight.h>

/* Reject inconsistent lengths before a fragment can overwrite the decode buffer.
 * A false result means the partial copy must not be submitted to the decoder. */
bool video_decode_unit_copy(unsigned char *buffer, size_t capacity, const DECODE_UNIT *unit);
