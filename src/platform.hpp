// SPDX-License-Identifier: LGPL-3.0-or-later
/** platform.hpp
 *  Platform detection helper.
 *
 *  Copyright 2019 dogtopus
 */

#pragma once

// Modern Arduino environment (>= 1.6.6)
#if defined(ARDUINO) && ARDUINO >= 10606
#define RDS4_ARDUINO
#include <Arduino.h>

// Check for teensy
#if defined(CORE_TEENSY) && (defined(__MK20DX128__) || \
                             defined(__MK20DX256__) || \
                             defined(__MKL26Z64__) || \
                             defined(__MK64FX512__) || \
                             defined(__MK66FX1M0__))
#define RDS4_TEENSY_3
#endif

// Older Arduino environment
#elif defined(ARDUINO)
#error "Legacy Arduino environment is not supported."

// Linux
#elif defined(RDS4_LINUX)
// for int*_t
#include <cstdint>
// for size_t
#include <cstddef>

#else
#error "Unknown/unsupported environment. If you are targeting for Linux system did you forget to set RDS4_LINUX?"

#endif
