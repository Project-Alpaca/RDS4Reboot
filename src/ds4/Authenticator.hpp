// SPDX-License-Identifier: LGPL-3.0-or-later
/** AuthenticatorDS4.hpp
 *  Authenticator for PS4 controllers via various back-ends.
 *
 *  Copyright 2019 dogtopus
 */

#pragma once

#include "utils/platform.hpp"
#include "api/internals.hpp"

// Sigh... https://github.com/arduino/arduino-builder/issues/15#issuecomment-145558252
#ifndef RDS4_MANUAL_CONFIG
#if __has_include(<PS4USB.h>)
#define RDS4_AUTH_USBH
#endif
#endif // RDS4_MANUAL_CONFIG

#ifdef RDS4_AUTH_USBH
#include <PS4USB.h>
// For auth structs
#include "Controller.hpp"
#endif

namespace rds4 {
namespace ds4 {

enum class BackendAuthState : uint8_t {
    OK,
    UNKNOWN_ERR,
    COMM_ERR,
    BUSY,
    NO_TRANSACTION,
};

class AuthenticatorBase {
public:
    /** Start authenticator. Does nothing by default. */
    virtual void begin() { };
    /** Check if the authenticator is connected and ready.
     *
     *  @return `true` if the authenticator is connected and ready.
     */
    virtual bool available() = 0;
    /** Check if the page size of challenge/response can be determined
     *  automatically (e.g.\ using licensedResetAuth).
     *
     *  @return `true` if the authenticator supports this.
     */
    virtual bool canFitPageSize() = 0;
    /** Report if the page size of challenge/response can be manually
     *  set to a specific value (e.g.\ direct access to the "security
     *  chip").
     *
     *  @return `true` if the authenticator supports this.
     */
    virtual bool canSetPageSize() = 0;
    /** Return true if the authenticator needs reset between authentications
     *
     *  @return `true` if the authenticator needs reset.
     */
    virtual bool needsReset() = 0;
    /** Automatically determine the size of page size. Only available on
     *  authenticators that supports this feature
     *
     *  @return `true` if the operation was successful.
     */
    virtual bool fitPageSize() = 0;
    /** Manually set the challenge page size.
     *  On authenticators that do not support this operation, this should do
     *  nothing and return `false`.
     *
     *  @param The page size. PS4 licensed controller "security chip" only
     *         allows page size < 256 bytes. Therefore it currently uses
     *         uint8_t. This might change when demanded.
     *  @return `true` if the operation was successful.
     */
    virtual bool setChallengePageSize(uint8_t size) {
        this->challengePageSize = size;
        /* Always returns true here. On authenticators that does not support
         * manual page size adjustments this should be overridden and return
         * false. */
        return true;
    }
    /** Manually set the response page size.
     *  On authenticators that do not support this operation, this should do
     *  nothing and return `false`.
     *
     *  @param The page size. PS4 licensed controller "security chip" only
     *         allows page size < 256 bytes. Therefore it currently uses
     *         uint8_t. This might change when demanded.
     *  @return `true` if the operation was successful.
     */
    virtual bool setResponsePageSize(uint8_t size) {
        this->responsePageSize = size;
        /* Always returns true here. On authenticators that does not support 
         * manual page size adjustments this should be overridden and return
         * false. */
        return true;
    }
    /** Determine if a page is the last page of challenge. Can be used as a
     *  cue for state transition of authentication handlers. If not
     *  applicable, always return `false`
     *
     *  @param The page to be determined.
     *  @return `true` if the page is the last page.
     */
    virtual bool endOfChallenge(uint8_t page) = 0;
    /** Determine if a page is the last page of response. Can be used as a
     *  cue for state transition of authentication handlers. If not
     *  applicable, always return `false`
     *
     *  @param The page to be determined.
     *  @return `true` if the page is the last page.
     */
    virtual bool endOfResponse(uint8_t page) = 0;
    /** Reset the authenticator. If not applicable, always return `true`.
     *
     *  @return `true` if reset is successful.
     */
    virtual bool reset() = 0;
    /** Write challenge page to the authenticator.
     *
     *  @param Page number.
     *  @param A pointer to the source buffer.
     *  @param Length of the source buffer (for sanity checks).
     *  @return Actual number of bytes copied.
     */
    virtual size_t writeChallengePage(uint8_t page, void *buf, size_t len) = 0;
    /** Read response page from the authenticator to a specified buffer.
     *
     *  @param Page number.
     *  @param A pointer to the destination buffer.
     *  @param Length of the destination buffer (for sanity checks).
     *  @return Actual number of bytes copied.
     */
    virtual size_t readResponsePage(uint8_t page, void *buf, size_t len) = 0;
    virtual uint8_t getChallengePageSize() {
        return this->challengePageSize;
    }
    virtual uint8_t getResponsePageSize() {
        return this->responsePageSize;
    }
    /** Check the current status of authentication.
     *
     *  @return Current authentication status.
     *  @see BackendAuthState
     */
    virtual BackendAuthState getStatus() = 0;

protected:
    uint8_t challengePageSize;
    uint8_t responsePageSize;
}; // AuthenticatorBase

class AuthenticatorNull : public AuthenticatorBase {
public:
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
    BackendAuthState getStatus() override { return BackendAuthState::UNKNOWN_ERR; }
};

#ifdef RDS4_AUTH_USBH

class AuthenticatorUSBH;

// TODO reading reports on licensed controllers don't work
/** Modified PS4USB class that adds basic support for some licensed PS4 controllers. */
class PS4USB2 : public ::PS4USB {
public:
    const uint16_t HORI_VID = 0x0f0d;
    const uint16_t HORI_PID_MINI = 0x00ee;
    const uint16_t RO_VID = 0x1430;
    const uint16_t RO_PID_GHPS4 = 0x07bb;

    PS4USB2(USB *p) : ::PS4USB(p), auth(nullptr) {};
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
    friend class AuthenticatorUSBH;
    bool VIDPIDOK(uint16_t vid, uint16_t pid) override {
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

    uint8_t OnInitSuccessful() override;
    void registerAuthenticator(AuthenticatorUSBH *auth);
private:
    AuthenticatorUSBH *auth;
};

/** DS4 authenticator that uses USB PS4 controllers as the backend via USB Host Shield 2.x library. */
class AuthenticatorUSBH : public AuthenticatorBase {
public:
    static const uint8_t PAYLOAD_MAX = 0x38;
    static const uint16_t CHALLENGE_SIZE = 0x100;
    static const uint16_t RESPONSE_SIZE = 0x410;
    AuthenticatorUSBH(PS4USB2 *donor);
    bool available() override;
    bool canFitPageSize() override { return true; }
    bool canSetPageSize() override { return false; }
    bool needsReset() override;
    bool fitPageSize() override;
    bool setChallengePageSize(uint8_t size) override { return false; }
    bool setResponsePageSize(uint8_t size) override { return false; }
    bool endOfChallenge(uint8_t page) override {
        return ((static_cast<uint16_t>(page)+1) * this->getChallengePageSize()) >= AuthenticatorUSBH::CHALLENGE_SIZE;
    }
    bool endOfResponse(uint8_t page) override {
        return ((static_cast<uint16_t>(page)+1) * this->getResponsePageSize()) >= AuthenticatorUSBH::RESPONSE_SIZE;
    }
    bool reset() override;
    size_t writeChallengePage(uint8_t page, void *buf, size_t len) override;
    size_t readResponsePage(uint8_t page, void *buf, size_t len) override;
    BackendAuthState getStatus() override;

protected:
    friend class PS4USB2;
    void onStateChange();
private:
    uint8_t getActualChallengePageSize(uint8_t page);
    uint8_t getActualResponsePageSize(uint8_t page);
    PS4USB2 *donor;
    uint8_t scratchPad[64];
    bool statusOverrideEnabled;
    uint32_t statusOverrideTransactionStartTime;
    bool statusOverrideInTransaction;
};

#endif // RDS4_AUTH_USBH
}
}
