// SPDX-License-Identifier: LGPL-3.0-or-later
/** AuthenticatorDS4.cpp
 *  Authenticator for PS4 controllers via various back-ends.
 *
 *  Copyright 2019 dogtopus
 */

#include "Authenticator.hpp"
#include "utils/utils.hpp"

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

size_t AuthenticatorUSBH::writeChallengePage(uint8_t page, const void *buf, size_t len) {
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


#if defined(RDS4_AUTH_NATIVE)

AuthenticatorNative::AuthenticatorNative():
        takingChallenge(false), responseStatus(ResponseStatus::CLEARED),
        ds4KeyLoaded(false), scratchPad{0}, identity{0} {
    mbedtls_rsa_init(&(this->key), MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
}

AuthenticatorNative::~AuthenticatorNative() {
    mbedtls_rsa_free(&(this->key));
}

bool AuthenticatorNative::available() {
    return this->ds4KeyLoaded;
}

bool AuthenticatorNative::fitPageSize() {
    if (!this->takingChallenge) {
        this->challengePageSize = 0x38;
        this->responsePageSize = 0x38;
        return true;
    }
    return false;
}

bool AuthenticatorNative::setChallengePageSize(uint8_t size) {
    if (!this->takingChallenge) {
        this->challengePageSize = size;
        return true;
    }
    return false;
}

bool AuthenticatorNative::setResponsePageSize(uint8_t size) {
    if (!this->takingChallenge) {
        this->responsePageSize = size;
        return true;
    }
    return false;
}

bool AuthenticatorNative::reset() {
    this->takingChallenge = false;
    this->responseStatus = ResponseStatus::CLEARED;
    this->performingAuth.clear();
    memset(&(this->scratchPad), 0, sizeof(this->scratchPad));
    return true;
}

size_t AuthenticatorNative::writeChallengePage(uint8_t page, const void *buf, size_t len) {
    // Immediately abort if DS4Key is not loaded or worker thread is running.
    if (!this->ds4KeyLoaded || this->performingAuth.get()) {
        return 0;
    }

    size_t start = this->responsePageSize * page;
    if (start >= AuthenticatorNative::CHALLENGE_SIZE) {
        return 0;
    }
    size_t remaining = (size_t) (AuthenticatorNative::CHALLENGE_SIZE - start);
    size_t length = (len < remaining) ? len : remaining;

    memcpy(&(this->scratchPad[start]), buf, length);

    if (this->endOfChallenge(page)) {
        // Signal the worker thread to start the auth process after last page write.
        this->performingAuth.set();
    }

    return length;
}

size_t AuthenticatorNative::readResponsePage(uint8_t page, void *buf, size_t len) {
    // Immediately abort if DS4Key is not loaded or worker thread is running.
    if (!this->ds4KeyLoaded || this->performingAuth.get() ||
            this->responseStatus != ResponseStatus::DONE) {
        return 0;
    }

    size_t start = this->responsePageSize * page;
    if (start >= AuthenticatorNative::RESPONSE_SIZE) {
        return 0;
    }
    size_t remaining = (size_t) (AuthenticatorNative::RESPONSE_SIZE - start);
    size_t length = (len < remaining) ? len : remaining;

    memcpy(buf, &(this->responseBuffer[start]), length);
    return 0;
}

BackendAuthState AuthenticatorNative::getStatus() {
    // taking challenge -> no transaction
    if (this->takingChallenge) {
        return BackendAuthState::NO_TRANSACTION;
    // not taking challenge but performing auth -> busy
    } else if (this->performingAuth.get()) {
        return BackendAuthState::BUSY;
    // not taking challenge, not performing auth
    } else if (this->responseStatus == ResponseStatus::DONE) {
        return BackendAuthState::OK;
    // not taking challenge, not performing auth, error when preparing response -> error
    } else if (this->responseStatus == ResponseStatus::ERROR) {
        return BackendAuthState::UNKNOWN_ERR;
    // not taking challenge, not performing auth, response is cleared -> no transaction
    } else if (this->responseStatus == ResponseStatus::CLEARED) {
        return BackendAuthState::NO_TRANSACTION;
    // unknown state -> error
    } else {
        return BackendAuthState::UNKNOWN_ERR;
    }
}

bool AuthenticatorNative::begin(const DS4FullKeyBlock &ds4key) {
    mbedtls_md_init(&(this->sha256));
    if (mbedtls_md_setup(&(this->sha256),mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0) != 0) {
        return false;
    }

    int importResult = mbedtls_rsa_import_raw(
        &(this->key),
        ds4key.identity.modulus, sizeof(ds4key.identity.modulus),
        ds4key.privateKey.p, sizeof(ds4key.privateKey.p),
        ds4key.privateKey.q, sizeof(ds4key.privateKey.q),
        nullptr, 0, // let mbedTLS generate d from p, q and e.
        ds4key.identity.exponent, sizeof(ds4key.identity.exponent)
    );
    // Do self test before deciding on whether the import was successful
    if (importResult == 0 && mbedtls_rsa_check_privkey(&(this->key)) == 0) {
        this->ds4KeyLoaded = true;
        return true;
    }
    return false;
}

void AuthenticatorNative::threadLoop() {
    uint8_t challengeSHA[32];

    while (1) {
        if (this->performingAuth.wait()) {
            this->responseStatus2 = ResponseStatus2::SHA256_BEGIN;
            if (mbedtls_md_starts(&(this->sha256)) != 0) {
                this->responseStatus = ResponseStatus::ERROR;
                this->performingAuth.clear();
                continue;
            };

            this->responseStatus2 = ResponseStatus2::SHA256_UPDATE;
            if (mbedtls_md_update(&(this->sha256), this->scratchPad, sizeof(this->scratchPad)) != 0) {
                this->responseStatus = ResponseStatus::ERROR;
                this->performingAuth.clear();
                continue;
            }

            this->responseStatus2 = ResponseStatus2::SHA256_DIGEST;
            if (mbedtls_md_finish(&(this->sha256), challengeSHA) != 0) {
                this->responseStatus = ResponseStatus::ERROR;
                this->performingAuth.clear();
                continue;
            }

            this->responseStatus2 = ResponseStatus2::RSA_SIGN;
            auto signResult = mbedtls_rsa_rsassa_pss_sign(
                &(this->key),
                // Stub RNG. Should be enough to let the function do signing but having
                // platform-specific RNG in rds4::utils and use it here would be nice.
                [](void *ctx_, unsigned char *buf, size_t len) {
                    memset(buf, 0, len);
                    return 0;
                },
                nullptr,
                MBEDTLS_RSA_PRIVATE,
                MBEDTLS_MD_SHA256,
                sizeof(challengeSHA),
                challengeSHA,
                this->scratchPad
            );
            if (signResult != 0) {
                this->responseStatus = ResponseStatus::ERROR;
                this->performingAuth.clear();
            } else {
                this->responseStatus = ResponseStatus::DONE;
                this->performingAuth.clear();
            }
        }
    }
}
#endif // RDS4_AUTH_NATIVE
} // namespace ds4
} // namespace rds4
