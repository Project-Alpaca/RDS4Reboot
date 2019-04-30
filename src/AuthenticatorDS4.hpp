/** AuthenticatorDS4.hpp
 *  Authenticator for PS4 controllers via various back-ends.
 *
 *  Copyright 2019 dogtopus
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "internals.hpp"

// Sigh... https://github.com/arduino/arduino-builder/issues/15#issuecomment-145558252
#if __has_include(<PS4USB.h>)
#define RDS4_AUTH_USBH
#include <PS4USB.h>
// For auth structs
#include "ControllerDS4.hpp"
#endif

namespace rds4 {

class AuthenticatorDS4Null : public AuthenticatorBase {
    void begin() { this->fitPageSize(); }
    bool available() override { return true; }
    bool canFitPageSize() override { return true; }
    bool canSetPageSize() override { return true; }
    bool needsReset() override { return false; }
    bool fitPageSize() override {
        this->challengePageSize = 0x38;
        this->responsePageSize = 0x38;
        return true;
    }
    bool endOfChallenge(uint8_t page) override { return true; }
    bool endOfResponse(uint8_t page) override { return true; }
    bool reset() override { return true; }
    size_t writeChallengePage(uint8_t page, void *buf, size_t len) { return len; }
    size_t readResponsePage(uint8_t page, void *buf, size_t len) { return 0; }
    AuthStatus getStatus() override { return AuthStatus::UNKNOWN_ERR; }
};

#ifdef RDS4_AUTH_USBH

/** Modified PS4USB class that adds basic support for some licensed PS4 controllers. */
class PS4USB2 : public ::PS4USB {
public:
    const uint16_t HORI_VID = 0x0f0d;
    const uint16_t HORI_PID_MINI = 0x00ee;
    const uint16_t RO_VID = 0x1430;
    const uint16_t RO_PID_GHPS4 = 0x07bb;

    PS4USB2(USB *p) : ::PS4USB(p) {};
    bool connected() {
        uint16_t v = ::HIDUniversal::VID;
        uint16_t p = ::HIDUniversal::PID;
        return ::HIDUniversal::isReady() and this->VIDPIDOK(v, p);
    }

    bool isLicensed(void) {
        return ::HIDUniversal::VID != PS4_VID;
    }

    bool isQuirky(void) {
        return (::HIDUniversal::VID == RO_VID and ::HIDUniversal::PID == RO_PID_GHPS4);
    }

protected:
    virtual bool VIDPIDOK(uint16_t vid, uint16_t pid) {
        return (( \
            vid == PS4_VID and ( \
                pid == PS4_PID or \
                pid == PS4_PID_SLIM \
            )
        ) or ( \
            vid == PS4USB2::HORI_VID and ( \
                pid == PS4USB2::HORI_PID_MINI \
            ) \
        ) or ( \
            vid == PS4USB2::RO_VID and ( \
                pid == PS4USB2::RO_PID_GHPS4 \
            ) \
        ));
    }
};

/** DS4 authenticator that uses USB PS4 controllers as the backend via USB Host Shield 2.x library. */
class AuthenticatorDS4USBH : public AuthenticatorBase {
public:
    static const uint8_t PAYLOAD_MAX = 0x38;
    static const uint16_t CHALLENGE_SIZE = 0x100;
    static const uint16_t RESPONSE_SIZE = 0x410;
    AuthenticatorDS4USBH(PS4USB2 *donor);
    bool available() override;
    bool canFitPageSize() override { return true; }
    bool canSetPageSize() override { return false; }
    bool needsReset() override;
    bool fitPageSize() override;
    bool setChallengePageSize(uint8_t size) override { return false; }
    bool setResponsePageSize(uint8_t size) override { return false; }
    bool endOfChallenge(uint8_t page) override {
        return (((uint16_t) page+1) * this->getChallengePageSize()) > AuthenticatorDS4USBH::CHALLENGE_SIZE;
    }
    bool endOfResponse(uint8_t page) override {
        return (((uint16_t) page+1) * this->getResponsePageSize()) > AuthenticatorDS4USBH::RESPONSE_SIZE;
    }
    bool reset() override;
    size_t writeChallengePage(uint8_t page, void *buf, size_t len) override;
    size_t readResponsePage(uint8_t page, void *buf, size_t len) override;
    AuthStatus getStatus() override;

private:
    uint8_t getActualChallengePageSize(uint8_t page);
    uint8_t getActualResponsePageSize(uint8_t page);
    bool wasConnected;
    PS4USB2 *donor;
    uint8_t scratchPad[64];
};

#endif // RDS4_AUTH_USBH
}
