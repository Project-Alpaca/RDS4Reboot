// SPDX-License-Identifier: LGPL-3.0-or-later
/** TransportDS4.hpp
 *  Various transport back-ends for PS4 controllers.
 *
 *  Copyright 2019 dogtopus
 */

#pragma once

#include "platform.hpp"
#include "internals.hpp"
#include "ControllerDS4.hpp"
#include "utils.hpp"

#ifdef RDS4_LINUX
#include <cstdio>
#endif

namespace rds4 {

enum class DS4AuthState : uint8_t {
    IDLE,
    NONCE_RECEIVED,
    WAIT_NONCE,
    WAIT_RESP,
    POLL_RESP,
    RESP_BUFFERED,
    RESP_UNLOADED,
    ERROR,
};

/** A wrapper for TransportDS4 family classes that adds ability to respond to
  * authentication requests.
  */
template <class TR, bool strictCRC=false>
class AuthenticationHandlerDS4 : public AuthenticationHandlerBase {
public:
    typedef void (*StateChangeCallback)(void);
    AuthenticationHandlerDS4(AuthenticatorBase *auth) : AuthenticationHandlerBase(auth),
                                                        state(DS4AuthState::IDLE),
                                                        page(-1),
                                                        seq(0),
                                                        scratchPad{0} {}
    void begin() override {
        this->auth->begin();
    }
    void update() override;
    void attachStateChangeCallback(StateChangeCallback callback) {
        _notifyStateChange = callback;
    }

protected:
    DS4AuthState state;
    int8_t page;
    uint8_t seq;
    // Maximum size for challenge/response
    uint8_t scratchPad[64];
    bool onGetReport(uint16_t value, uint16_t index, uint16_t length) override;
    bool onSetReport(uint16_t value, uint16_t index, uint16_t length) override;
    void notifyStateChange(void) {
        if (this->_notifyStateChange != nullptr) {
            (*this->_notifyStateChange)();
        }
    }
    StateChangeCallback _notifyStateChange;
};

template <class TR, bool strictCRC>
void AuthenticationHandlerDS4<TR, strictCRC>::update() {
    auto *pkt = reinterpret_cast<ds4_auth_t *>(&(this->scratchPad));
    if (this->auth->available()) {
        switch (this->state) {
            // Got nonce (challenge) from host
            case DS4AuthState::NONCE_RECEIVED:
                RDS4_DBG_PRINTLN("AuthenticationHandlerDS4: consuming nonce");
                // If this is the first page and the auth device is resettable, reset it
                if (this->page == 0) {
                    // Use auto fit if available, otherwise manually set to maximum size if possible
                    // TODO verify buffer size 0x38 on A7105
                    if (this->auth->canSetPageSize() and not this->auth->canFitPageSize()) {
                        RDS4_DBG_PRINTLN("set pagesize to maximum");
                        this->auth->setChallengePageSize(sizeof(pkt->data));
                        this->auth->setResponsePageSize(sizeof(pkt->data));
                    }
                    // Reset also fits the buffer size if possible
                    if (this->auth->needsReset()) {
                        RDS4_DBG_PRINTLN("reset");
                        this->auth->reset();
                    // Otherwise trigger auto fit explicitly
                    } else if (this->auth->canFitPageSize()) {
                        RDS4_DBG_PRINTLN("auto fit");
                        this->auth->fitPageSize();
                    }
                }
                // Submit the page to auth device
                if (this->auth->writeChallengePage(this->page, &(pkt->data), sizeof(pkt->data))) {
                    if (this->auth->endOfChallenge(this->page)) {
                        RDS4_DBG_PRINTLN("last cpage");
                        this->state = DS4AuthState::WAIT_RESP;
                    } else {
                        // wait for more
                        this->state = DS4AuthState::WAIT_NONCE;
                    }
                } else {
                    RDS4_DBG_PRINTLN("write err");
                    this->state = DS4AuthState::ERROR;
                }
                break;
            case DS4AuthState::POLL_RESP: {
                auto as = this->auth->getStatus();
                RDS4_DBG_PRINTLN("AuthenticationHandlerDS4: checking auth status");
                switch (as) {
                    // Authenticator is ready to answer the challenge.
                    case AuthStatus::OK: {
                        // buffer the first response packet
                        auto *pkt = reinterpret_cast<ds4_auth_t *>(&(this->scratchPad));
                        RDS4_DBG_PRINTLN("ok");
                        pkt->type = ControllerDS4::GET_RESPONSE;
                        pkt->seq = this->seq;
                        pkt->page = 0;
                        pkt->crc32 = strictCRC ? crc32(this->scratchPad, sizeof(*pkt) - sizeof(pkt->crc32)) : 0;
                        this->page = 0;

                        if (this->auth->readResponsePage(this->page, &(pkt->data), sizeof(pkt->data))) {
                            this->state = DS4AuthState::RESP_BUFFERED;
                        } else {
                            RDS4_DBG_PRINTLN("err");
                            this->state = DS4AuthState::ERROR;
                        }
                        break;
                    }
                    // Authenticator is busy, wait for some more time.
                    case AuthStatus::BUSY:
                        // Wait until host polls again.
                        RDS4_DBG_PRINTLN("busy");
                        this->state = DS4AuthState::WAIT_RESP;
                        break;
                    // Something went wrong
                    default:
                        RDS4_DBG_PRINTLN("err");
                        this->state = DS4AuthState::ERROR;
                        break;
                }
                break;
            }
            case DS4AuthState::RESP_UNLOADED: {
                RDS4_DBG_PRINTLN("AuthenticationHandlerDS4: producing resp");
                if (this->auth->endOfResponse(this->page)) {
                    RDS4_DBG_PRINTLN("last rpage");
                    this->state = DS4AuthState::IDLE;
                    this->page = -1;
                    break;
                }
                auto *pkt = reinterpret_cast<ds4_auth_t *>(&(this->scratchPad));
                RDS4_DBG_PRINTLN("next");
                this->page++;
                pkt->type = ControllerDS4::GET_RESPONSE;
                pkt->seq = this->seq;
                pkt->page = this->page;
                pkt->crc32 = strictCRC ? crc32(this->scratchPad, sizeof(*pkt) - sizeof(pkt->crc32)) : 0;
                // clear the buffer just in case
                memset(&(pkt->data), 0, sizeof(pkt->data));
                if (this->auth->readResponsePage(this->page, &(pkt->data), sizeof(pkt->data))) {
                    this->state = DS4AuthState::RESP_BUFFERED;
                } else {
                    RDS4_DBG_PRINTLN("err");
                    this->state = DS4AuthState::ERROR;
                }
                break;
            }
            case DS4AuthState::IDLE:
            default:
                break;
        }
    }
}

template <class TR, bool strictCRC>
bool AuthenticationHandlerDS4<TR, strictCRC>::onSetReport(uint16_t value, uint16_t index, uint16_t length) {
    TR *tr = static_cast<TR *>(this);
    if ((value >> 8) == 0x03) {
        switch (value & 0xff) {
            case ControllerDS4::SET_CHALLENGE: {
                auto *pkt = reinterpret_cast<ds4_auth_t *>(&(this->scratchPad));
                RDS4_DBG_PRINTLN("AuthenticationHandlerDS4: SET_CHALLENGE");
                if (tr->check(pkt, sizeof(*pkt)) != sizeof(*pkt)) {
                    RDS4_DBG_PRINTLN("wrong size");
                    return false;
                }
                // sanity check
                if (pkt->type != ControllerDS4::SET_CHALLENGE) {
                    RDS4_DBG_PRINT("wrong magic ");
                    RDS4_DBG_PHEX(pkt->type);
                    RDS4_DBG_PRINT("\n");
                    return false;
                }
                // Page 0 acts like a reset
                if (pkt->page == 0) {
                    RDS4_DBG_PRINTLN("reset");
                    this->page = 0;
                    this->seq = pkt->seq;
                    this->state = DS4AuthState::NONCE_RECEIVED;
                    this->notifyStateChange();
                } else if (this->state == DS4AuthState::WAIT_NONCE) {
                    // If currently waiting for more nonce, make sure the order is consistent. Otherwise go to error state.
                    if (pkt->seq == this->seq and pkt->page == this->page + 1) {
                        RDS4_DBG_PRINTLN("cont");
                        this->page++;
                        this->state = DS4AuthState::NONCE_RECEIVED;
                        this->notifyStateChange();
                        this->notifyStateChange();
                    } else {
                        RDS4_DBG_PRINTLN("ooo");
                        this->state = DS4AuthState::ERROR;
                    }
                } else {
                    RDS4_DBG_PRINTLN("err");
                    this->page = -1;
                    this->state = DS4AuthState::ERROR;
                }
                break;
            }
            default:
                // unknown cmd, stall
                return false;
        }
    }
    return true;
}

template <class TR, bool strictCRC>
bool AuthenticationHandlerDS4<TR, strictCRC>::onGetReport(uint16_t value, uint16_t index, uint16_t length) {
    TR *tr = static_cast<TR *>(this);
    if ((value >> 8) == 0x03) {
        switch (value & 0xff) {
            case ControllerDS4::GET_RESPONSE:
                // Will be processed in update()
                if (this->state == DS4AuthState::RESP_BUFFERED) {
                    this->state = DS4AuthState::RESP_UNLOADED;
                    this->notifyStateChange();
                } else {
                    // TODO do we need to clean the buffer?
                    this->state = DS4AuthState::ERROR;
                }
                tr->reply(&scratchPad, sizeof(ds4_auth_t));
                break;
            case ControllerDS4::GET_AUTH_STATUS: {
                // Use a separate buffer here to make sure we don't overwrite buffered response.
                ds4_auth_status_t pkt = {0};
                pkt.type = ControllerDS4::GET_AUTH_STATUS;
                pkt.seq = this->seq;
                pkt.crc32 = strictCRC ? crc32(&pkt, sizeof(pkt) - sizeof(pkt.crc32)) : 0;
                switch (this->state) {
                    // Already responding to the host (aka. ready)
                    case DS4AuthState::RESP_BUFFERED:
                    case DS4AuthState::RESP_UNLOADED:
                        pkt.status = 0x00; // ok
                        break;
                    // Still waiting for the auth device
                    case DS4AuthState::WAIT_RESP:
                    case DS4AuthState::POLL_RESP:
                        pkt.status = 0x10; // busy
                        // notify the other end that the host polled us
                        this->state = DS4AuthState::POLL_RESP;
                        this->notifyStateChange();
                        break;
                    // Something went wrong or not in a transaction
                    case DS4AuthState::ERROR:
                        pkt.status = 0xf0;
                        break;
                    default:
                        pkt.status = 0x01; // not in a transaction
                        break;
                }
                tr->reply(&pkt, sizeof(pkt));
                break;
            }
            case ControllerDS4::GET_AUTH_PAGE_SIZE: {
                auto *ps = reinterpret_cast<ds4_auth_page_size_t *>(&(this->scratchPad));
                memset(ps, 0, sizeof(*ps));
                ps->type = ControllerDS4::GET_AUTH_PAGE_SIZE;
                ps->size_challenge = this->auth->getChallengePageSize();
                ps->size_response = this->auth->getResponsePageSize();
                tr->reply(ps, sizeof(*ps));
                break;
            }
            default:
                // unknown cmd, stall
                return false;
        }
    }
    return true;
}

template <class TR>
class FeatureConfigurator {
protected:
    static const uint8_t RESPONSE[48];
    bool onGetReport(uint16_t value, uint16_t index, uint16_t length);
};

template <class TR>
const uint8_t FeatureConfigurator<TR>::RESPONSE[] PROGMEM = {
    0x03, 0x21, 0x27, 0x04, 0x4d, 0x00, 0x2c, 0x56,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x0d, 0x0d, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

template <class TR>
bool FeatureConfigurator<TR>::onGetReport(uint16_t value, uint16_t index, uint16_t length) {
    TR *tr = static_cast<TR *>(this);
    if (value == 0x0303) {
        tr->reply(&FeatureConfigurator::RESPONSE, sizeof(FeatureConfigurator::RESPONSE));
        return true;
    } else {
        return false;
    }
}

#if defined(RDS4_ARDUINO) && defined(RDS4_TEENSY_3)
typedef struct {
    union {
        struct {
            uint8_t bmRequestType;
            uint8_t bmRequest;
        };
        uint16_t wRequestAndType;
    };
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_pkt_t;

/** Tansport backend for teensy 3.x/LC boards. Requires patched teensyduino core library */
class TransportDS4Teensy;
class TransportDS4Teensy : public TransportBase, public AuthenticationHandlerDS4<TransportDS4Teensy>, public FeatureConfigurator<TransportDS4Teensy> {
public:
    TransportDS4Teensy(AuthenticatorBase *auth) : AuthenticationHandlerDS4(auth) {
        TransportDS4Teensy::inst = this;
        usb_ds4stub_on_get_report = &(TransportDS4Teensy::frCallbackGet);
        usb_ds4stub_on_set_report = &(TransportDS4Teensy::frCallbackSet);
    }
    void begin() override {
        AuthenticationHandlerDS4<TransportDS4Teensy>::begin();
    }
    bool available() override;
    uint8_t send(const void *buf, uint8_t len) override;
    uint8_t sendBlocking(const void *buf, uint8_t len) override;
    uint8_t recv(void *buf, uint8_t len) override;

protected:
    friend class AuthenticationHandlerDS4<TransportDS4Teensy>;
    friend class FeatureConfigurator<TransportDS4Teensy>;
    // Copy data to DMA buffer
    uint8_t check(void *buf, uint8_t len) override;
    // Unload data from DMA buffer
    uint8_t reply(const void *buf, uint8_t len) override;
    bool onGetReport(uint16_t value, uint16_t index, uint16_t length) override;
    bool onSetReport(uint16_t value, uint16_t index, uint16_t length) override;
    static int frCallbackGet(void *setup_ptr, uint8_t *data, uint32_t *len);
    static int frCallbackSet(void *setup_ptr, uint8_t *data);
    // Feature request DMA buffer
    static uint8_t *frBuffer;
    static uint32_t *frSize;
    // Since this class can only be instantiated once, this should be fine.
    static TransportDS4Teensy *inst;
};

#elif defined(RDS4_ARDUINO) && defined(USBCON)

#include "PluggableUSB.h"
#include "HID.h"

#if !defined(_USING_HID)
#error "Pluggable HID required"
#endif

typedef struct {
    InterfaceDescriptor hid;
    HIDDescDescriptor desc;
    EndpointDescriptor in;
    EndpointDescriptor out;
} DS4HIDDescriptor;

#if defined(ARDUINO_ARCH_AVR)
typedef uint8_t usb_eptype_t;
#elif defined(ARDUINO_ARCH_SAMD)
typedef uint32_t usb_eptype_t;
const usb_eptype_t EP_TYPE_INTERRUPT_IN = USB_ENDPOINT_TYPE_INTERRUPT | USB_ENDPOINT_IN(0);
const usb_eptype_t EP_TYPE_INTERRUPT_OUT = USB_ENDPOINT_TYPE_INTERRUPT | USB_ENDPOINT_OUT(0);
#endif

/** Tansport backend for Arduino PluggableUSB-compatible boards. */
class TransportDS4PUSB : public PluggableUSBModule, public TransportBase, public AuthenticationHandlerDS4<TransportDS4PUSB> {
public:
    TransportDS4PUSB(AuthenticatorBase *auth) : AuthenticationHandlerDS4(auth),
                                                PluggableUSBModule(2, 1, epType) {
        this->epType = {EP_TYPE_INTERRUPT_IN, EP_TYPE_INTERRUPT_OUT};
        PluggableUSB().plug(this);
        
    }
    int begin(void) override { return; }
    uint8_t send(const void *buf, uint8_t len) override;
    uint8_t recv(void *buf, uint8_t len) override;
    bool available() override;

protected:
    // PluggableUSB API
    bool setup(USBSetup& setup);
    int getInterface(uint8_t* interfaceCount) override;
    int getDescriptor(USBSetup& setup) override;
    uint8_t reply(const void *buf, uint8_t len) override;
    uint8_t check(void *buf, uint8_t len) override;

private:
    inline uint8_t getOutEP() { return this->pluggedEndpoint; }
    inline uint8_t getInEP() { return this->pluggedEndpoint - 1; }
    usb_eptype_t epType[2];
    // TODO do we need these?
    uint8_t protocol;
    uint8_t idle;
};

#endif
#if 0
// This is outdated, update soon
#ifdef RDS4_LINUX

class TransportNull : public TransportBase {
    bool sendAvailable() { return false; }
    bool recvAvailable() { return false; }
    size_t sendReport(const void *buf, size_t len) { return 0; }
    size_t recvReport(void *buf, size_t len) { return 0; }
};

class TransportStdio : public TransportBase {
    bool sendAvailable() { return true; }
    bool recvAvailable() { return false; }
    size_t sendReport(const void *buf, size_t len) {
        return fwrite(buf, sizeof(uint8_t), len, stdout);
    }
    size_t recvReport(void *buf, size_t len) { return 0; }
};

#endif // RDS4_MOCK
#endif
} // namespace rds4
