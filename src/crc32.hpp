#pragma once

#include <stddef.h>
#include <stdint.h>

namespace fastpng {
    // Uses the fast crc32 instructions if target machine supports sse4.2, else falls back to a scalar algorithm
    uint32_t crc32(const uint8_t* bytes, size_t len, uint32_t initial = 0);
}
