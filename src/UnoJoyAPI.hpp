/** UnoJoyAPI.hpp
 *  UnoJoy API definitions.
 *
 *  Copyright 2019 dogtopus
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "internals.hpp"

namespace rds4 {

typedef struct {
    // keys byte 0
    uint8_t triangleOn : 1;
    uint8_t circleOn : 1;
    uint8_t squareOn : 1;
    uint8_t crossOn : 1;
    uint8_t l1On : 1;
    uint8_t l2On : 1;
    uint8_t l3On : 1;
    uint8_t r1On : 1;

    // keys byte 1
    uint8_t r2On : 1;
    uint8_t r3On : 1;
    uint8_t selectOn : 1;
    uint8_t startOn : 1;
    uint8_t homeOn : 1;
    uint8_t dpadLeftOn : 1;
    uint8_t dpadUpOn : 1;
    uint8_t dpadRightOn : 1;

    // keys byte 2
    uint8_t dpadDownOn : 1;
    uint8_t reserved : 7;
    
    // sticks
    uint8_t leftStickX : 8;
    uint8_t leftStickY : 8;
    uint8_t rightStickX : 8;
    uint8_t rightStickY : 8;
} dataForController_t;

template <class T>
class UnoJoyAPI {
public:
    void setControllerData(dataForController_t buf);
};

extern dataForController_t getBlankDataForController();

} // rds4
