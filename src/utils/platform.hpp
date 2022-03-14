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

// TODO other arduino-mbed platforms
#elif defined(ARDUINO_ARCH_RP2040)
#define RDS4_ARDUINO_MBED

#endif

// Older Arduino environment
#elif defined(ARDUINO)
#error "Legacy Arduino environment is not supported."

#else
#error "Unknown/unsupported environment."

#endif
