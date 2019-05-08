// SPDX-License-Identifier: LGPL-3.0-or-later
/** AuthenticatorDS4.cpp
 *  Authenticator for PS4 controllers via various back-ends.
 *
 *  Copyright 2019 dogtopus
 */

#include "AuthenticatorDS4.hpp"
#include "utils.hpp"

#ifdef RDS4_LINUX
// for memcpy(), etc.
#include <cstring>
#endif

namespace rds4 {

#ifdef RDS4_AUTH_USBH

uint8_t PS4USB2::OnInitSuccessful() {
    if (this->VIDPIDOK(::HIDUniversal::VID, ::HIDUniversal::PID)) {
        PS4Parser::Reset();
        if (this->auth != nullptr) {
            this->auth->onStateChange();
        }
        if (this->isLicensed()) {
            this->setLed(Blue);
        }
    }
    return 0;
}

void PS4USB2::registerAuthenticator(AuthenticatorDS4USBH *auth) {
    this->auth = auth;
}

AuthenticatorDS4USBH::AuthenticatorDS4USBH(PS4USB2 *donor) : donor(donor) {
    donor->registerAuthenticator(this);
}

bool AuthenticatorDS4USBH::available() {
    auto state = this->donor->connected();
    return state;
}

bool AuthenticatorDS4USBH::needsReset() {
    return this->donor->isLicensed();
}

bool AuthenticatorDS4USBH::reset() {
    return this->fitPageSize();
}

bool AuthenticatorDS4USBH::fitPageSize() {
    if (this->donor->isLicensed()) {
        if (this->donor->GetReport(0, 0, 0x03, ControllerDS4::GET_AUTH_PAGE_SIZE, 8, this->scratchPad) == 0) {
            auto pgsize = (ds4_auth_page_size_t *) &(this->scratchPad);
            // basic sanity check
            if (pgsize->size_challenge > AuthenticatorDS4USBH::PAYLOAD_MAX or pgsize->size_response > AuthenticatorDS4USBH::PAYLOAD_MAX) {
                return false;
            }
            RDS4_DBG_PRINT("AuthenticatorDS4USBH: fit: nonce=");
            RDS4_DBG_PHEX(pgsize->size_challenge);
            RDS4_DBG_PRINT(" resp=");
            RDS4_DBG_PHEX(pgsize->size_response);
            RDS4_DBG_PRINT("\n");
            this->challengePageSize = pgsize->size_challenge;
            this->responsePageSize = pgsize->size_response;
            return true;
        } else {
            RDS4_DBG_PRINTLN("AuthenticatorDS4USBH: fit: comm error");
            return false;
        }
    } else {
        RDS4_DBG_PRINT("AuthenticatorDS4USBH: fit: is ds4");
        this->challengePageSize = AuthenticatorDS4USBH::PAYLOAD_MAX;
        this->responsePageSize = AuthenticatorDS4USBH::PAYLOAD_MAX;
        return true;
    }
}

size_t AuthenticatorDS4USBH::writeChallengePage(uint8_t page, void *buf, size_t len) {
    auto authbuf = reinterpret_cast<ds4_auth_t *>(&(this->scratchPad));
    auto expected = this->getActualChallengePageSize(page);
    RDS4_DBG_PRINTLN("AuthenticatorDS4USBH: writing page");
    // Insufficient data
    if (len < expected) {
        RDS4_DBG_PRINTLN("buf too small");
    }
    
    authbuf->type = ControllerDS4::SET_CHALLENGE;
    // seq=0 seems to work on all controllers I tested. Leave this as-is for now.
    authbuf->seq = 1;
    authbuf->page = page;
    authbuf->sbz = 0;
    memcpy(&authbuf->data, buf, expected);
    // CRC32 is a must-have for official controller, not sure about licensed ones.
    authbuf->crc32 = crc32(this->scratchPad, sizeof(*authbuf) - sizeof(authbuf->crc32));
    if (this->donor->SetReport(0, 0, 0x03, ControllerDS4::SET_CHALLENGE, sizeof(*authbuf), this->scratchPad) != 0) {
        RDS4_DBG_PRINTLN("comm error");
        return 0;
    }
    RDS4_DBG_PHEX(expected);
    RDS4_DBG_PRINTLN(" bytes written");
    return expected;
}

size_t AuthenticatorDS4USBH::readResponsePage(uint8_t page, void *buf, size_t len) {
    auto authbuf = (ds4_auth_t *) &(this->scratchPad);
    auto expected = this->getActualResponsePageSize(page);
    // Insufficient space for target buffer
    RDS4_DBG_PRINTLN("AuthenticatorDS4USBH: reading page");
    if (len < expected) {
        RDS4_DBG_PRINTLN("buf too small");
        return 0;
    }
    if (this->donor->GetReport(0, 0, 0x03, ControllerDS4::GET_RESPONSE, sizeof(*authbuf), this->scratchPad) != 0) {
        RDS4_DBG_PRINTLN("comm error");
        return 0;
    }
    // Sanity check
    // (`page` has usage other than sanity check for, e.g., "security chip" authenticator.)
    if (authbuf->page != page) {
        RDS4_DBG_PRINT("page mismatch exp=");
        RDS4_DBG_PHEX(page);
        RDS4_DBG_PRINT(" act=");
        RDS4_DBG_PHEX(authbuf->page);
        RDS4_DBG_PRINT("\n");
        return 0;
    }
    memcpy(buf, &authbuf->data, expected);
    RDS4_DBG_PHEX(expected);
    RDS4_DBG_PRINTLN(" bytes read");
    return expected;
}

AuthStatus AuthenticatorDS4USBH::getStatus() {
    auto rslbuf = (ds4_auth_status_t *) &(this->scratchPad);
    RDS4_DBG_PRINTLN("AuthenticatorDS4USBH: getting status");
    memset(rslbuf, 0, sizeof(*rslbuf));
    if (this->donor->GetReport(0, 0, 0x03, ControllerDS4::GET_AUTH_STATUS, sizeof(*rslbuf), this->scratchPad) != 0) {
        RDS4_DBG_PRINTLN("comm err");
        return AuthStatus::COMM_ERR;
    }
    switch (rslbuf->status) {
        case 0x00:
            // TODO what about GH dongle?
            // The GH dongle takes about 2 seconds to sign the challenge
            // if (this->donor->isQuirky()) { ...; return AuthStatus::BUSY; }
            // GH dongle also has wrong return value for busy (returns NO_TRANSACTION)
            // Perhaps skip status all together?
            RDS4_DBG_PRINTLN("ok");
            return AuthStatus::OK;
            break;
        case 0x01:
            RDS4_DBG_PRINTLN("not in transaction");
            return AuthStatus::NO_TRANSACTION;
            break;
        case 0x10:
            RDS4_DBG_PRINTLN("busy");
            return AuthStatus::BUSY;
            break;
        default:
            RDS4_DBG_PRINT("unk err ");
            RDS4_DBG_PHEX(rslbuf->status);
            RDS4_DBG_PRINT("\n");
            return AuthStatus::UNKNOWN_ERR;
    }
}

uint8_t AuthenticatorDS4USBH::getActualChallengePageSize(uint8_t page) {
    uint16_t remaining = AuthenticatorDS4USBH::CHALLENGE_SIZE - (uint16_t) this->challengePageSize * page;
    return remaining > this->challengePageSize ? this->challengePageSize : (uint8_t) remaining;
}

uint8_t AuthenticatorDS4USBH::getActualResponsePageSize(uint8_t page) {
    uint16_t remaining = AuthenticatorDS4USBH::RESPONSE_SIZE - (uint16_t) this->responsePageSize * page;
    return remaining > this->responsePageSize ? this->responsePageSize : (uint8_t) remaining;
}

void AuthenticatorDS4USBH::onStateChange() {
    RDS4_DBG_PRINTLN("AuthenticatorDS4USBH: Hotplug detected, re-fitting buffer");
    this->fitPageSize();
}

#endif // RDS4_AUTH_USBH

} // namespace rds4
