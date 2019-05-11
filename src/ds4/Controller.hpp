// SPDX-License-Identifier: LGPL-3.0-or-later
/** ControllerDS4.hpp
 *  High-level report handling API for PS4 controllers.
 *
 *  Copyright 2019 dogtopus
 */

#pragma once

#include "utils/platform.hpp"
#include "api/internals.hpp"
#include "api/UnoJoyAPI.hpp"

namespace rds4 {
namespace ds4 {

struct TouchFrame {
    uint8_t seq; // 34
    uint32_t pos[2]; // 35-42
} __attribute__((packed));

struct InputReport {
    uint8_t type; // 0
    union {
        struct {
            uint8_t stick_l_x; // 1
            uint8_t stick_l_y; // 2
            uint8_t stick_r_x; // 3
            uint8_t stick_r_y; // 4
        };
        uint8_t sticks[4];
    };
    uint8_t buttons[3]; // 5-7
    union {
        struct {
            uint8_t trigger_l; // 8
            uint8_t trigger_r; // 9
        };
        uint8_t triggers[2];
    };
    uint16_t sensor_timestamp; // 10-11
    uint8_t battery; // 12
    uint8_t u13; // 13
    int16_t accel_z; // 14-15
    int16_t accel_y; // 16-17
    int16_t accel_x; // 18-19
    int16_t gyro_x; // 20-21
    int16_t gyro_y; // 22-23
    int16_t gyro_z; // 24-25
    uint32_t u26; // 26-29
    uint8_t state_ext; // 30
    uint16_t u31; // 31-32
    uint8_t tp_available_frame; // 33
    TouchFrame frames[3]; // 34-60
    uint8_t padding[3]; // 61-62 (63?)
} __attribute__((packed));

struct FeedbackReport {
    uint8_t type; // 0
    uint8_t flags; // 1
    uint8_t padding1[2]; // 2-3
    uint8_t rumble_right; // 4
    uint8_t rumble_left; // 5
    uint8_t led_color[3]; // 6-8
    uint8_t led_flash_on; // 9
    uint8_t led_flash_off; // 10
    uint8_t padding[21]; // 11-31
} __attribute__((packed));

struct AuthPageSizeReport {
    uint8_t type;
    uint8_t u1;
    uint8_t size_challenge;
    uint8_t size_response;
    uint8_t u4[4]; // crc32?
} __attribute__((packed)) ;

struct AuthReport {
    uint8_t type; // 0
    uint8_t seq; // 1
    uint8_t page; // 2
    uint8_t sbz; // 3
    uint8_t data[56]; // 4-59
    uint32_t crc32; // 60-63
} __attribute__((packed));

struct AuthStatusReport {
    uint8_t type; // 0
    uint8_t seq; // 1
    uint8_t status; // 2  0x10 = not ready, 0x00 = ready
    uint8_t padding[9]; // 3-11
    uint32_t crc32; // 12-15
} __attribute__((packed));

class Controller : public api::ControllerBase {
public:
    enum : uint8_t {
        ROT_MAIN = 0,
    };
    enum : uint8_t {
        KEY_SQR = 0,
        KEY_XRO,
        KEY_CIR,
        KEY_TRI,
        KEY_L1,
        KEY_R1,
        KEY_L2,
        KEY_R2,
        KEY_SHR,
        KEY_OPT,
        KEY_L3,
        KEY_R3,
        KEY_PS,
        KEY_TP,
    };
    enum : uint8_t {
        AXIS_LX = 0,
        AXIS_LY,
        AXIS_RX,
        AXIS_RY,
        AXIS_L2,
        AXIS_R2,
    };
    enum : uint8_t {
        IN_REPORT = 0x1,
        OUT_FEEDBACK = 0x5,
        SET_CHALLENGE = 0xf0,
        GET_RESPONSE,
        GET_AUTH_STATUS,
        GET_AUTH_PAGE_SIZE,
    };
    Controller(api::Transport *backend);
    void begin() override;
    void update();
    bool sendReport() override;
    bool sendReportBlocking() override;
    bool setRotary8Pos(uint8_t code, api::Rotary8Pos value) override;
    bool setKey(uint8_t code, bool action) override;
    bool setAxis(uint8_t code, uint8_t value) override;
    bool setAxis16(uint8_t code, uint16_t value) override;

    bool setKeyUniversal(api::Key code, bool action) override;
    bool setDpadUniversal(api::Dpad value) override;
    bool setStick(api::Stick index, uint8_t x, uint8_t y) override;
    bool setTrigger(api::Key code, uint8_t value) override;

    bool setTouchpad(uint8_t slot, uint8_t pos, bool pressed, uint8_t seq, uint16_t x, uint16_t y);
    bool setTouchEvent(uint8_t pos, bool pressed, uint16_t x=0, uint16_t y=0);
    bool finalizeTouchEvent();
    void clearTouchEvents();

    bool hasValidFeedback();
    uint8_t getRumbleIntensityRight();
    uint8_t getRumbleIntensityLeft();
    uint32_t getLEDRGB();
    uint8_t getLEDDelayOn();
    uint8_t getLEDDelayOff();

private:
    InputReport report;
    FeedbackReport feedback;
    uint8_t currentTouchSeq;
    void incReportCtr();
    static const uint8_t keyLookup[];
    bool sendReport_(bool blocking);
};

template <api::Dpad NS=api::Dpad::C, api::Dpad WE=api::Dpad::C>
class ControllerSOCD : public Controller, public api::SOCDBehavior<ControllerSOCD<NS, WE>, NS, WE>, public api::UnoJoyAPI<ControllerSOCD<NS, WE>> {
public:
    ControllerSOCD(api::Transport *backend) : Controller(backend) {};
};

} // namespace ds4
} // namespace rds4
