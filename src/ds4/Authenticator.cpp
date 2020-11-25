// SPDX-License-Identifier: LGPL-3.0-or-later
/** AuthenticatorDS4.cpp
 *  Authenticator for PS4 controllers via various back-ends.
 *
 *  Copyright 2019 dogtopus
 */

#include "Authenticator.hpp"
#include "utils/utils.hpp"

#ifdef RDS4_LINUX
// for memcpy(), etc.
#include <cstring>
#endif

namespace rds4 {
namespace ds4 {

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

void PS4USB2::registerAuthenticator(AuthenticatorUSBH *auth) {
    this->auth = auth;
}

AuthenticatorUSBH::AuthenticatorUSBH(PS4USB2 *donor) : donor(donor), statusOverrideEnabled(false), statusOverrideTransactionStartTime(0), statusOverrideInTransaction(false) {
    donor->registerAuthenticator(this);
}

bool AuthenticatorUSBH::available() {
    auto state = this->donor->connected();
    return state;
}

bool AuthenticatorUSBH::needsReset() {
    return this->donor->isLicensed();
}

bool AuthenticatorUSBH::reset() {
    return this->fitPageSize();
}

bool AuthenticatorUSBH::fitPageSize() {
    if (this->donor->isLicensed()) {
        if (this->donor->GetReport(0, 0, 0x03, Controller::GET_AUTH_PAGE_SIZE, 8, this->scratchPad) == 0) {
            auto pgsize = (AuthPageSizeReport *) &(this->scratchPad);
            // basic sanity check
            if (pgsize->size_challenge > AuthenticatorUSBH::PAYLOAD_MAX or pgsize->size_response > AuthenticatorUSBH::PAYLOAD_MAX) {
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
        this->challengePageSize = AuthenticatorUSBH::PAYLOAD_MAX;
        this->responsePageSize = AuthenticatorUSBH::PAYLOAD_MAX;
        return true;
    }
}

size_t AuthenticatorUSBH::writeChallengePage(uint8_t page, void *buf, size_t len) {
    auto authbuf = reinterpret_cast<AuthReport *>(&(this->scratchPad));
    auto expected = this->getActualChallengePageSize(page);
    RDS4_DBG_PRINTLN("AuthenticatorDS4USBH: writing page");
    // Insufficient data
    if (len < expected) {
        RDS4_DBG_PRINTLN("buf too small");
    }
    
    authbuf->type = Controller::SET_CHALLENGE;
    // seq=0 seems to work on all controllers I tested. Leave this as-is for now.
    authbuf->seq = 1;
    authbuf->page = page;
    authbuf->sbz = 0;
    memcpy(&authbuf->data, buf, expected);
    // CRC32 is a must-have for official controller, not sure about licensed ones.
    authbuf->crc32 = utils::crc32(this->scratchPad, sizeof(*authbuf) - sizeof(authbuf->crc32));
    if (this->donor->SetReport(0, 0, 0x03, Controller::SET_CHALLENGE, sizeof(*authbuf), this->scratchPad) != 0) {
        RDS4_DBG_PRINTLN("comm error");
        return 0;
    }
    RDS4_DBG_PHEX(expected);
    RDS4_DBG_PRINTLN(" bytes written");
    // Guitar Hero Dongle hack
    if (this->statusOverrideEnabled and this->endOfChallenge(page)) {
        RDS4_DBG_PRINTLN("gh hack timer start");
        this->statusOverrideInTransaction = true;
        this->statusOverrideTransactionStartTime = millis();
    }
    return expected;
}

size_t AuthenticatorUSBH::readResponsePage(uint8_t page, void *buf, size_t len) {
    auto authbuf = (AuthReport *) &(this->scratchPad);
    auto expected = this->getActualResponsePageSize(page);
    // Insufficient space for target buffer
    RDS4_DBG_PRINTLN("AuthenticatorDS4USBH: reading page");
    if (len < expected) {
        RDS4_DBG_PRINTLN("buf too small");
        return 0;
    }
    if (this->donor->GetReport(0, 0, 0x03, Controller::GET_RESPONSE, sizeof(*authbuf), this->scratchPad) != 0) {
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
    // Guitar Hero Dongle hack
    if (this->statusOverrideEnabled and this->endOfResponse(page)) {
        RDS4_DBG_PRINTLN("gh hack end transaction");
        this->statusOverrideInTransaction = false;
    }
    return expected;
}

BackendAuthState AuthenticatorUSBH::getStatus() {
    auto rslbuf = (AuthStatusReport *) &(this->scratchPad);
    RDS4_DBG_PRINTLN("AuthenticatorDS4USBH: getting status");
    if (this->statusOverrideEnabled) {
        RDS4_DBG_PRINTLN("gh hack enabled");
        // wait for 2 seconds since the GH dongle takes about 2 seconds to sign the challenge
        if (this->statusOverrideInTransaction and millis() - this->statusOverrideTransactionStartTime > 2000) {
            return BackendAuthState::OK;
        } else if (this->statusOverrideInTransaction) {
            return BackendAuthState::BUSY;
        } else {
            return BackendAuthState::NO_TRANSACTION;
        }
    }
    memset(rslbuf, 0, sizeof(*rslbuf));
    if (this->donor->GetReport(0, 0, 0x03, Controller::GET_AUTH_STATUS, sizeof(*rslbuf), this->scratchPad) != 0) {
        RDS4_DBG_PRINTLN("comm err");
        return BackendAuthState::COMM_ERR;
    }
    switch (rslbuf->status) {
        case 0x00:
            RDS4_DBG_PRINTLN("ok");
            return BackendAuthState::OK;
            break;
        case 0x01:
            RDS4_DBG_PRINTLN("not in transaction");
            return BackendAuthState::NO_TRANSACTION;
            break;
        case 0x10:
            RDS4_DBG_PRINTLN("busy");
            return BackendAuthState::BUSY;
            break;
        default:
            RDS4_DBG_PRINT("unk err ");
            RDS4_DBG_PHEX(rslbuf->status);
            RDS4_DBG_PRINT("\n");
            return BackendAuthState::UNKNOWN_ERR;
    }
}

uint8_t AuthenticatorUSBH::getActualChallengePageSize(uint8_t page) {
    uint16_t remaining = AuthenticatorUSBH::CHALLENGE_SIZE - (uint16_t) this->challengePageSize * page;
    return remaining > this->challengePageSize ? this->challengePageSize : (uint8_t) remaining;
}

uint8_t AuthenticatorUSBH::getActualResponsePageSize(uint8_t page) {
    uint16_t remaining = AuthenticatorUSBH::RESPONSE_SIZE - (uint16_t) this->responsePageSize * page;
    return remaining > this->responsePageSize ? this->responsePageSize : (uint8_t) remaining;
}

void AuthenticatorUSBH::onStateChange() {
    RDS4_DBG_PRINTLN("AuthenticatorDS4USBH: Hotplug detected, re-fitting buffer");
    this->fitPageSize();
    this->statusOverrideEnabled = this->donor->isQuirky();
}

#endif // RDS4_AUTH_USBH

} // namespace ds4
} // namespace rds4
