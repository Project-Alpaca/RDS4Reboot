/** ControllerDS4.cpp
 *  High-level report handling API for PS4 controllers.
 *
 *  Copyright 2019 dogtopus
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ControllerDS4.hpp"
#include "utils.hpp"

#ifdef RDS4_LINUX
// for memcpy(), etc.
#include <cstring>
#endif

namespace rds4 {

ControllerDS4::ControllerDS4(TransportBase *backend) : ControllerBase(backend), currentTouchSeq(0) { /* pass */ };

void ControllerDS4::begin() {
    this->backend->begin();
    memset(&(this->report), 0, sizeof(this->report));
    this->report.type = 0x01;
    // Center the D-Pad
    this->setRotary8Pos(0, Rotary8Pos::C);
    // Analog sticks
    memset(this->report.sticks, 0x80, sizeof(this->report.sticks));
    // Touch
    this->clearTouchEvents();
    // Ext TODO
    this->report.state_ext = 0x08;
    this->report.battery = 0xff;
}

void ControllerDS4::update() {
    if (this->backend->available()) {
        // TODO
        this->backend->recv(&(this->feedback), sizeof(this->feedback));
    }
}

bool ControllerDS4::hasValidFeedback() {
    return this->feedback.type == 0x05;
}

bool ControllerDS4::sendReport() {
    // https://www.psdevwiki.com/ps4/DS4-BT#0x11
    this->report.sensor_timestamp = ((millis() * 150) & 0xffff);
    auto actual = this->backend->send(&(this->report), sizeof(this->report));
    if (actual != sizeof(this->report)) {
        return false;
    } else {
        this->incReportCtr();
        // Touch events are not persisted across each report.
        this->clearTouchEvents();
        return true;
    }
    return false;
}

inline void ControllerDS4::incReportCtr() {
    this->report.buttons[2] += 4;
}

bool ControllerDS4::setRotary8Pos(uint8_t code, Rotary8Pos value) {
    if (code != 0) {
        return false;
    }
    this->report.buttons[0] ^= this->report.buttons[0] & 0x0f;
    this->report.buttons[0] |= static_cast<uint8_t>(value);
    return true;
}

bool ControllerDS4::setKey(uint8_t code, bool action) {
    if (code < ControllerDS4::KEY_SQR or code > ControllerDS4::KEY_TP) {
        // key does not exist
        return false;
    }
    // Offset for button bitfield
    code += 4;
    // keycode structure: 000BBbbb
    // B: byte offset
    // b: bit offset
    if (action) {
        // pressed
        this->report.buttons[(code >> 3) & 3] |= 1 << (code & 7);
    } else {
        //released
        this->report.buttons[(code >> 3) & 3] &= ~(1 << (code & 7));
    }
    return true;
}

bool ControllerDS4::setAxis(uint8_t code, uint8_t value) {
    if (code >= ControllerDS4::AXIS_LX and code <= ControllerDS4::AXIS_RY) {
        this->report.sticks[code] = value;
    } else if (code >= ControllerDS4::AXIS_L2 and code <= ControllerDS4::AXIS_R2) {
        this->report.triggers[code-4] = value;
    } else {
        return false;
    }
    return true;
}

bool ControllerDS4::setAxis16(uint8_t code, uint16_t value) {
    // TODO accel and gyro support
    // No 16-bit axes at the moment, stub this
    return false;
}

static inline bool tpGetState(uint32_t tp) {
    return (bool) ((~(tp >> 7)) & 1);
}

static inline uint8_t tpGetId(uint32_t tp) {
    return (uint8_t) (tp & 0x7f);
}

static inline uint16_t tpGetX(uint32_t tp) {
    return ((tp >> 8) & 0xfff);
}

static inline uint16_t tpGetY(uint32_t tp) {
    return ((tp >> 20) & 0xfff);
}

static inline void tpSetState(uint32_t *tp, bool val) {
    if (val) {
        (*tp) |= 0x80;
    } else {
        (*tp) &= 0xffffff7ful;
    }
}

bool ControllerDS4::setTouchpad(uint8_t slot, uint8_t pos, bool pressed, uint8_t seq, uint16_t x, uint16_t y) {
    // TODO Bluetooth has different event buffer size
    if (slot >= (sizeof(this->report.frames) / sizeof(ds4_touch_frame_t)) || pos > 1) {
        return false;
    }
    this->report.frames[slot].pos[pos] = ((y & 0xfff) << 20) | ((x & 0xfff) << 8) | ((~(int8_t) pressed) << 7) | (seq & 0x7f);
    this->report.frames[slot].seq++;
    return true;
}

bool ControllerDS4::setTouchEvent(uint8_t pos, bool pressed, uint16_t x, uint16_t y) {
    return this->setTouchpad(this->report.tp_available_frame, pos, pressed, this->currentTouchSeq, x, y);
}

bool ControllerDS4::finalizeTouchEvent() {
    if (this->report.tp_available_frame < 3) {
        this->report.tp_available_frame++;
        this->currentTouchSeq++;
        return true;
    } else {
        return false;
    }
}

void ControllerDS4::clearTouchEvents() {
    this->report.tp_available_frame = 0;
    for (uint8_t i=0; i<3; i++) {
        this->report.frames[i].pos[0] = 1 << 7;
        this->report.frames[i].pos[1] = 1 << 7;
    }
}

uint8_t ControllerDS4::getRumbleIntensityRight() {
    return this->feedback.rumble_right;
}

uint8_t ControllerDS4::getRumbleIntensityLeft() {
    return this->feedback.rumble_left;
}

uint8_t ControllerDS4::getLEDDelayOn() {
    return this->feedback.led_flash_on;
}

uint8_t ControllerDS4::getLEDDelayOff() {
    return this->feedback.led_flash_off;
}

uint32_t ControllerDS4::getLEDRGB() {
    // LED data is in the format of 0x00RRGGBB, same as the format Adafruit_NeoPixel accepts.
    return (this->feedback.led_color[0] << 16 | this->feedback.led_color[1] << 8 | this->feedback.led_color[2]);
}


}
