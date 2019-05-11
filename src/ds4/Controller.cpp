// SPDX-License-Identifier: LGPL-3.0-or-later
/** ControllerDS4.cpp
 *  High-level report handling API for PS4 controllers.
 *
 *  Copyright 2019 dogtopus
 */

#include "Controller.hpp"

#include "utils/utils.hpp"

#ifdef RDS4_LINUX
// for memcpy(), etc.
#include <cstring>
#endif

namespace rds4 {
namespace ds4 {

const uint8_t Controller::keyLookup[static_cast<uint8_t>(api::Key::_COUNT)] = {
    Controller::KEY_CIR,
    Controller::KEY_XRO,
    Controller::KEY_TRI,
    Controller::KEY_SQR,
    Controller::KEY_L1,
    Controller::KEY_R1,
    Controller::KEY_L2,
    Controller::KEY_R2,
    Controller::KEY_L3,
    Controller::KEY_R3,
    Controller::KEY_PS,
    Controller::KEY_SHR,
    Controller::KEY_OPT,
};

Controller::Controller(api::Transport *backend) : api::Controller(backend), currentTouchSeq(0) { /* pass */ };

void Controller::begin() {
    this->backend->begin();
    memset(&(this->report), 0, sizeof(this->report));
    this->report.type = 0x01;
    // Center the D-Pad
    this->setRotary8Pos(0, api::Rotary8Pos::C);
    // Analog sticks
    memset(this->report.sticks, 0x80, sizeof(this->report.sticks));
    // Touch
    this->clearTouchEvents();
    // Ext TODO
    this->report.state_ext = 0x08;
    this->report.battery = 0xff;
}

void Controller::update() {
    if (this->backend->available()) {
        // TODO
        this->backend->recv(&(this->feedback), sizeof(this->feedback));
    }
}

bool Controller::hasValidFeedback() {
    return this->feedback.type == 0x05;
}

inline bool Controller::sendReport_(bool blocking) {
    // https://www.psdevwiki.com/ps4/DS4-BT#0x11
    this->report.sensor_timestamp = ((millis() * 150) & 0xffff);
    uint8_t actual;
    if (blocking) {
        actual = this->backend->sendBlocking(&(this->report), sizeof(this->report));
    } else {
        actual = this->backend->send(&(this->report), sizeof(this->report));
    }
    if (actual != sizeof(this->report)) {
        return false;
    } else {
        this->incReportCtr();
        if (this->report.tp_available_frame > 1) {
            // copy the last frame to the first slot and nuke the rest
            memcpy(&this->report.frames[0], &this->report.frames[this->report.tp_available_frame - 1], sizeof(this->report.frames[0]));
            for (uint8_t i=1; i<3; i++) {
                this->report.frames[i].seq = 0;
                this->report.frames[i].pos[0] = 1 << 7;
                this->report.frames[i].pos[1] = 1 << 7;
            }
            this->report.tp_available_frame = 1;
        }
        return true;
    }
    return false;
}

bool Controller::sendReport() {
    return this->sendReport_(false);
}

bool Controller::sendReportBlocking() {
    return this->sendReport_(true);
}

inline void Controller::incReportCtr() {
    this->report.buttons[2] += 4;
}

bool Controller::setRotary8Pos(uint8_t code, api::Rotary8Pos value) {
    if (code != 0) {
        return false;
    }
    this->report.buttons[0] ^= this->report.buttons[0] & 0x0f;
    this->report.buttons[0] |= static_cast<uint8_t>(value);
    return true;
}

bool Controller::setKey(uint8_t code, bool action) {
    if (code < Controller::KEY_SQR or code > Controller::KEY_TP) {
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

bool Controller::setAxis(uint8_t code, uint8_t value) {
    if (code >= Controller::AXIS_LX and code <= Controller::AXIS_RY) {
        this->report.sticks[code] = value;
    } else if (code >= Controller::AXIS_L2 and code <= Controller::AXIS_R2) {
        this->report.triggers[code-4] = value;
    } else {
        return false;
    }
    return true;
}

bool Controller::setAxis16(uint8_t code, uint16_t value) {
    // TODO accel and gyro support
    // No 16-bit axes at the moment, stub this
    return false;
}

bool Controller::setKeyUniversal(api::Key code, bool action) {
    if (code == api::Key::_COUNT) {
        return false;
    }
    auto ds4Code = this->keyLookup[static_cast<uint8_t>(code)];
    switch (code) {
        case api::Key::LTrigger:
            this->setAxis(Controller::AXIS_L2, action ? 0xff : 0x0);
            break;
        case api::Key::RTrigger:
            this->setAxis(Controller::AXIS_R2, action ? 0xff : 0x0);
            break;
        default:
            break;
    }
    this->setKey(ds4Code, action);
    return true;
}

bool Controller::setDpadUniversal(api::Dpad value) {
    return this->setDpad(0, value);
}

bool Controller::setStick(api::Stick index, uint8_t x, uint8_t y) {
    switch (index) {
        case api::Stick::L:
            this->setAxis(Controller::AXIS_LX, x);
            this->setAxis(Controller::AXIS_LY, y);
            break;
        case api::Stick::R:
            this->setAxis(Controller::AXIS_RX, x);
            this->setAxis(Controller::AXIS_RY, y);
            break;
    }
    return true;
}

bool Controller::setTrigger(api::Key code, uint8_t value) {
    auto ds4Code = this->keyLookup[static_cast<uint8_t>(code)];
    switch (code) {
        case api::Key::LTrigger:
            this->setAxis(Controller::AXIS_L2, value);
            break;
        case api::Key::RTrigger:
            this->setAxis(Controller::AXIS_R2, value);
            break;
        default:
            break;
    }
    this->setKey(ds4Code, value);
    return true;
}

bool Controller::setTouchpad(uint8_t slot, uint8_t pos, bool pressed, uint8_t seq, uint16_t x, uint16_t y) {
    // TODO Bluetooth has different event buffer size
    if (slot >= (sizeof(this->report.frames) / sizeof(TouchFrame)) || pos > 1) {
        return false;
    }
    this->report.frames[slot].pos[pos] = ((y & 0xfff) << 20) | ((x & 0xfff) << 8) | ((!pressed) << 7) | (seq & 0x7f);
    this->report.frames[slot].seq++;
    return true;
}

bool Controller::setTouchEvent(uint8_t pos, bool pressed, uint16_t x, uint16_t y) {
    return this->setTouchpad(this->report.tp_available_frame, pos, pressed, this->currentTouchSeq, x, y);
}

bool Controller::finalizeTouchEvent() {
    if (this->report.tp_available_frame < 3) {
        this->report.tp_available_frame++;
        this->currentTouchSeq++;
        return true;
    } else {
        return false;
    }
}

void Controller::clearTouchEvents() {
    this->report.tp_available_frame = 0;
    for (uint8_t i=0; i<3; i++) {
        this->report.frames[i].seq = 0;
        this->report.frames[i].pos[0] = 1 << 7;
        this->report.frames[i].pos[1] = 1 << 7;
    }
}

uint8_t Controller::getRumbleIntensityRight() {
    return this->feedback.rumble_right;
}

uint8_t Controller::getRumbleIntensityLeft() {
    return this->feedback.rumble_left;
}

uint8_t Controller::getLEDDelayOn() {
    return this->feedback.led_flash_on;
}

uint8_t Controller::getLEDDelayOff() {
    return this->feedback.led_flash_off;
}

uint32_t Controller::getLEDRGB() {
    // LED data is in the format of 0x00RRGGBB, same as the format Adafruit_NeoPixel accepts.
    return (this->feedback.led_color[0] << 16 | this->feedback.led_color[1] << 8 | this->feedback.led_color[2]);
}

}
}
