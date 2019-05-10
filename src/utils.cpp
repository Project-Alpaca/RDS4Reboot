// SPDX-License-Identifier: LGPL-3.0-or-later
/** utils.cpp
 *  Various utility functions.
 *
 *  Copyright 2019 dogtopus
 */

#include "utils.hpp"

namespace rds4 {
namespace utils {

static const uint32_t crc32_table_4b[] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c,
};

uint32_t crc32(void *buf, size_t len) {
    uint32_t result = 0xfffffffful;
    for (size_t i=0; i<len; i++) {
        result ^= ((uint8_t *) buf)[i];
        result = crc32_table_4b[result & 0xf] ^ (result >> 4);
        result = crc32_table_4b[result & 0xf] ^ (result >> 4);
    }
    return result ^ 0xfffffffful;
}

}
}
