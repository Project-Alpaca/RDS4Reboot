/** utils.hpp
 *  Various utility functions.
 *
 *  Copyright 2019 dogtopus
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

// For sysdep
#include "internals.hpp"

#ifdef RDS4_DEBUG

#if defined(RDS4_LINUX)

#define RDS4_DBG_DEFINED
#define RDS4_DBG_PHEX(val) printf("%x", val)
#define RDS4_DBG_PRINT(str) printf("%s", str)
#define RDS4_DBG_PRINTLN(str) printf("%s\n", str)

#elif defined(RDS4_ARDUINO)

#ifndef RDS4_ARDUINO_DBG_IO
#define RDS4_ARDUINO_DBG_IO Serial
#endif // RDS4_ARDUINO_DBG_IO

#define RDS4_DBG_DEFINED
#define RDS4_DBG_PHEX(val) RDS4_ARDUINO_DBG_IO.print(val, HEX)
#define RDS4_DBG_PRINT(str) RDS4_ARDUINO_DBG_IO.print(str)
#define RDS4_DBG_PRINTLN(str) RDS4_ARDUINO_DBG_IO.println(str)

#endif // defined(RDS4_ARDUINO)
#endif // RDS4_DEBUG

#ifndef RDS4_DBG_DEFINED
#define RDS4_DBG_PHEX(val) do {} while (0)
#define RDS4_DBG_PRINT(str) do {} while (0)
#define RDS4_DBG_PRINTLN(str) do {} while (0)
#endif

namespace rds4 {
    uint32_t crc32(void *buf, size_t len);
}
