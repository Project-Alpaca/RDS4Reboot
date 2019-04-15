/** internals.hpp
 *  RDS4Reboot API definitions.
 *
 *  Copyright 2019 dogtopus
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

// TODO: More documentation

// Modern Arduino environment (>= 1.6.6)
#if defined(ARDUINO) && ARDUINO >= 10606
#define RDS4_ARDUINO

// Check for teensy
#if defined(CORE_TEENSY) && !defined(_AVR_)
#define RDS4_TEENSY_3
#endif

#include <Arduino.h>

// Older Arduino environment
#elif defined(ARDUINO) && ARDUINO >= 100
#define RDS4_ARDUINO_LEGACY
#warning "C++11 features may not be enabled on Arduino < 1.6.6. Build may fail."

// Legacy Arduino environment (Does it even work?)
//#elif defined(ARDUINO)
//#include <WProgram.h>

// Linux
#elif defined(RDS4_LINUX)
// for int*_t
#include <cstdint>
// for size_t
#include <cstddef>

#else
#error "Unknown/unsupported environment. If you are targeting for Linux system did you forget to set RDS4_LINUX?"

#endif

namespace rds4 {

/** Base class for a Transport class.
 *  A Transport object should be able to handle high-level report transferring
 *  (high-level as hiding details about device discovery and initialization, if
 *  applicable), as well as be able to dispatch feature requests.
 *  It should know the protocol of the device to the level that
 *  allows it to correctly dispatch data to the right recipient (host device or
 *  other classes on this device).
 *
 *  Currently the size of packet is limited to 255 bytes per packet since even
 *  full DS4 report is way less than 255 bytes long (without audio). This may
 *  change in the future when demanded.
 */
class TransportBase {
public:
    /** Start transport backend */
    virtual void begin() = 0;
    /** Check if there are packets that are available for receiving.
     *
     *  @return `true` if available, `false` if no packet is available.
     */
    virtual bool available() = 0;
    /** Send data through the link. This method shall be implemented as
     *  non-blocking, meaning it immediately returns if data transmission
     *  is not possible.
     *
     *  @param The buffer that contains data to send.
     *  @param The length of the supplied buffer.
     *  @return The number of actual bytes sent.
     */
    virtual uint8_t send(const void *buf, uint8_t len) = 0;
    /** Receive data from the link. This method shall be implemented as
     *  non-blocking, meaning it immediately returns if there is no data to
     *  receive or any error occurred.
     *
     *  @param The buffer for receiving the data.
     *  @param The length of the supplied buffer.
     *  @return The number of actual bytes received.
     */
    virtual uint8_t recv(void *buf, uint8_t len) = 0;
    // TODO blocking API for (RT)OSes
protected:
    // Feature request API. Intended for internal use. If developing CRTPs
    // (for auth, etc.) one must set the CRTP class as friend in order to
    // use these.

    /** Reply to a feature request. For use within the implementation
     *  classes only.
     *
     *  @param The buffer that contains data to send.
     *  @param The length of the supplied buffer.
     *  @return The number of actual bytes sent.
     */
    virtual uint8_t reply(const void *buf, uint8_t len) = 0;
    /** Checkout the supplied data from a feature request. For use within
     *  the implementation classes only.
     *
     *  @param The buffer for receiving the data.
     *  @param The length of the supplied buffer.
     *  @return The number of actual bytes received.
     */
    virtual uint8_t check(void *buf, uint8_t len) = 0;
    virtual bool onGetReport(uint16_t value, uint16_t index, uint16_t length) = 0;
    virtual bool onSetReport(uint16_t value, uint16_t index, uint16_t length) = 0;
}; // TransportBase

enum class AuthStatus : uint8_t {
    OK,
    UNKNOWN_ERR,
    COMM_ERR,
    BUSY,
    NO_TRANSACTION,
};

class AuthenticatorBase {
public:
    virtual void begin() = 0;
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
     *  @see AuthStatus
     */
    virtual AuthStatus getStatus() = 0;

protected:
    uint8_t challengePageSize;
    uint8_t responsePageSize;
}; // AuthenticatorBase


/** Optional authentication feature for a Transport object. Should be a
  * mixin/wrapper (CRTP) of a specific implementation of Transport class.
  */
class AuthenticationHandlerBase {
public:
    AuthenticationHandlerBase(AuthenticatorBase *auth) {
        this->auth = auth;
    }
    virtual void begin() = 0;
    /** Check for updates on authentication.
      * Can be called in a super-loop or called on-demand on an (RT)OS (when e.g. there is an update event on the host side).
      */
    virtual void update() = 0;

protected:
    virtual bool onGetReport(uint16_t value, uint16_t index, uint16_t length) = 0;
    virtual bool onSetReport(uint16_t value, uint16_t index, uint16_t length) = 0;
    AuthenticatorBase *auth;
}; // AuthenticationHandlerBase

enum class Rotary8Pos : uint8_t {
    N = 0,
    NE,
    E,
    SE,
    S,
    SW,
    W,
    NW,
    C,
};

using Dpad8Pos = Rotary8Pos;

class ControllerBase {
public:
    ControllerBase(TransportBase *backend) {
        this->backend = backend;
    }
    virtual void begin() = 0;
    virtual bool sendReport() = 0;
    virtual bool setRotary8Pos(uint8_t code, Rotary8Pos value) = 0;
    virtual bool setKey(uint8_t code, bool action) = 0;
    virtual bool setAxis(uint8_t code, uint8_t value) = 0;
    virtual bool setAxis16(uint8_t code, uint16_t value) = 0;

    // Helpers
    inline bool setDpad(uint8_t code, Dpad8Pos value) {
        return this->setRotary8Pos(code, value);
    }
    inline bool pressKey(uint8_t code) {
        return this->setKey(code, true);
    }
    inline bool releaseKey(uint8_t code) {
        return this->setKey(code, false);
    }

protected:
    TransportBase *backend;
}; // ControllerBase

} // namespace rds4
