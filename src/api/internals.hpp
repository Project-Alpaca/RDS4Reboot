// SPDX-License-Identifier: LGPL-3.0-or-later
/** internals.hpp
 *  RDS4Reboot API definitions.
 *
 *  Copyright 2019 dogtopus
 */

#pragma once

#include "utils/platform.hpp"

// TODO: More documentation

namespace rds4 {
namespace api {

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
class Transport {
public:
    /** Start transport backend. Does nothing by default. */
    virtual void begin() { };
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
    /** Send data through the link (blocking). The blocking timeout is
     *  implementation-specific
     *
     *  @param The buffer that contains data to send.
     *  @param The length of the supplied buffer.
     *  @return The number of actual bytes sent.
     */
    virtual uint8_t sendBlocking(const void *buf, uint8_t len) = 0;
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
    virtual uint8_t setOutgoingFeatureReport(const void *buf, uint8_t len) = 0;
    /** Checkout the supplied data from a feature request. For use within
     *  the implementation classes only.
     *
     *  @param The buffer for receiving the data.
     *  @param The length of the supplied buffer.
     *  @return The number of actual bytes received.
     */
    virtual uint8_t getIncomingFeatureReport(void *buf, uint8_t len) = 0;
    virtual bool onGetReport(uint16_t value, uint16_t index, uint16_t length) = 0;
    virtual bool onSetReport(uint16_t value, uint16_t index, uint16_t length) = 0;
}; // TransportBase


class EndpointResponder {
protected:
    virtual int onGetReport(uint8_t type, uint8_t id) = 0;
    virtual int onSetReport(uint8_t type, uint8_t id) = 0;
};


enum class Rotary8Pos : uint8_t {
    N = 0,
    NE,
    E,
    SE,
    S,
    SW,
    W,
    NW,
    C = 8,
    NUL = 8,
};

enum class Key : uint8_t {
    A = 0, B, X, Y, LButton, RButton, LTrigger, RTrigger, LStick, RStick,
    Home, Select, Start, _COUNT,
};

enum class Stick : uint8_t {
    L = 0, R,
};

using Dpad = Rotary8Pos;

class Controller {
public:
    Controller(Transport *backend) {
        this->backend = backend;
    }
    /** Initialize the report buffer and transport back-end. */
    virtual void begin() = 0;
    /** Send the current report (non-blocking).
     *  @return `true` if successful.
     */
    virtual bool sendReport() = 0;
    /** Send the current report (blocking).
     *  @return `true` if successful.
     */
    virtual bool sendReportBlocking() = 0;
    /** Set state for a rotary encoder (8 positions with null state).
     *  @param index of the encoder. This is implementation-specific.
     *  @param value of the encoder.
     *  @return `true` if successful.
     *  @see Rotary8Pos
     */
    virtual bool setRotary8Pos(uint8_t code, Rotary8Pos value) = 0;
    /** Set state for a push button/key.
     *  @param the key code. This is implementation-specific.
     *  @param `true` if the key is pressed.
     *  @return `true` if successful.
     */
    virtual bool setKey(uint8_t code, bool action) = 0;
    /** Set state for an analog axis (8-bit).
     *  @param the index of axis. This is implementation-specific.
     *  @param the value of the axis.
     *  @return `true` if successful.
     *  @see setAxis16()
     */
    virtual bool setAxis(uint8_t code, uint8_t value) = 0;
    /** Set state for an analog axis (16-bit).
     *  @param the index of axis. This is implementation-specific.
     *  @param the value of the axis.
     *  @return `true` if successful.
     *  @see setAxis()
     */
    virtual bool setAxis16(uint8_t code, uint16_t value) = 0;

    // Universal APIs
    virtual bool setKeyUniversal(Key code, bool action) = 0;
    virtual bool setDpadUniversal(Dpad value) = 0;
    virtual bool setStick(Stick index, uint8_t x, uint8_t y) = 0;
    virtual bool setTrigger(Key code, uint8_t value) = 0;

    // Helpers
    /** Set state for a D-pad. Equivalent to setRotary8Pos().
     *  @param index of the D-pad. This is implementation-specific.
     *  @param value of the D-pad.
     *  @return `true` if successful.
     *  @see Dpad8Pos
     *  @see setRotary8Pos()
     */
    inline bool setDpad(uint8_t code, Dpad value) {
        return this->setRotary8Pos(code, value);
    }
    /** Set a key/push button to pressed state.
     *  @param the key code.
     *  @return `true` if successful.
     *  @see releaseKey()
     *  @see setKey()
     */
    inline bool pressKey(uint8_t code) {
        return this->setKey(code, true);
    }
    /** Set a key/push button to released state.
     *  @param the key code.
     *  @return `true` if successful.
     *  @see pressKey()
     *  @see setKey()
     */
    inline bool releaseKey(uint8_t code) {
        return this->setKey(code, false);
    }

    inline bool pressKeyUniversal(Key code) {
        return this->setKeyUniversal(code, true);
    }

    inline bool releaseKeyUniversal(Key code) {
        return this->setKeyUniversal(code, false);
    }

protected:
    Transport *backend;
}; // ControllerBase

/** A simple SOCD cleaner mixin. Can be attached to a ControllerBase-compatible
 *  class by creating a new subclass of it as `C` and inheriting
 *  `SOCDBehavior<C>`.
 *  The exact behavior of the cleaner is specified via template parameter `NS`
 *  (both Up (north) and Down (south) are pressed) and `WE` (both Left (west)
 *  and Right (east) are pressed). When `NS` is set to either `Dpad8Pos::N` or
 *  `Dpad8Pos::S`, the cleaner only keeps Up or Down press while discards the
 *  opposite press.
 *  Similarily, when `WE` is set to either `Dpad8Pos::W` or `Dpad8Pos::E`, only
 *  Left or Right will be kept. Any other parameters, including `Dpad8Pos::C`
 *  will be treated as neutral and the cleaner removes presses of both
 *  directions.
 */
template <class C, Dpad NS, Dpad WE>
class SOCDBehavior {
public:
    /** Do SOCD cleaning and set state for a D-pad.
     *  @param index of the D-pad. This is implementation-specific.
     *  @param True if the Up (North) button is pressed.
     *  @param True if the Right (East) button is pressed.
     *  @param True if the Down (South) button is pressed.
     *  @param True if the Left (West) button is pressed.
     *  @return `true` if successful.
     *  @see Dpad8Pos
     */
    bool setDpadSOCD(uint8_t code, bool n, bool e, bool s, bool w) {
        auto *cobj = static_cast<C *>(this);
        return cobj->setDpad(code, this->doCleaning(n, e, s, w));
    }
    bool setDpadUniversalSOCD(bool n, bool e, bool s, bool w) {
        auto *cobj = static_cast<C *>(this);
        return cobj->setDpadUniversal(this->doCleaning(n, e, s, w));
    }
private:
    Dpad doCleaning(bool n, bool e, bool s, bool w) {
        auto pos = Dpad::C;
        // Clean the input
        if (n and s) {
            switch (NS) {
                case Dpad::N:
                    s = false;
                    break;
                case Dpad::S:
                    n = false;
                    break;
                case Dpad::C:
                default:
                    s = false;
                    n = false;
            }
        }
        if (w and e) {
            switch (WE) {
                case Dpad::W:
                    e = false;
                    break;
                case Dpad::E:
                    w = false;
                    break;
                case Dpad::C:
                default:
                    e = false;
                    w = false;
            }
        }
        // Map cleaned input to D-Pad positions
        if (n) {
            // NE
            if (e) {
                pos = Dpad::NE;
            // NW
            } else if (w) {
                pos = Dpad::NW;
            // N only
            } else {
                pos = Dpad::N;
            }
        } else if (s) {
            // SE
            if (e) {
                pos = Dpad::SE;
            // SW
            } else if (w) {
                pos = Dpad::SW;
            // S only
            } else {
                pos = Dpad::S;
            }
        // W only
        } else if (w) {
            pos = Dpad::W;
        // E only
        } else if (e) {
            pos = Dpad::E;
        // Centered/neutral
        } else {
            pos = Dpad::C;
        }
        return pos;
    }
}; // SOCDBehavior
} // namespace api
} // namespace rds4
