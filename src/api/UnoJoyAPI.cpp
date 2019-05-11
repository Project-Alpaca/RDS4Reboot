// SPDX-License-Identifier: LGPL-3.0-or-later
/** UnoJoyAPI.cpp
 *  UnoJoy API definitions.
 *
 *  Copyright 2019 dogtopus
 */

#include "utils/platform.hpp"
#include "UnoJoyAPI.hpp"

namespace rds4 {
namespace api {

dataForController_t getBlankDataForController() {
    dataForController_t buf;
    memset(&buf, 0, sizeof(buf));
    buf.leftStickX = 0x80;
    buf.leftStickY = 0x80;
    buf.rightStickX = 0x80;
    buf.rightStickY = 0x80;
    return buf;
}

}
}
