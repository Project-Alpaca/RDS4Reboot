// SPDX-License-Identifier: LGPL-3.0-or-later
/** TransportDS4.cpp
 *  Various transport back-ends for PS4 controllers.
 *
 *  Copyright 2019 dogtopus
 */

#include "TransportDS4.hpp"
#include "utils.hpp"

#if defined(RDS4_ARDUINO) && defined(RDS4_TEENSY_3)

#include "usb_dev.h"
#include "usb_ds4stub.h"

// TODO find a way to get rid of this or translate to static const
#define DS4_INTERFACE      0	// DS4
#define DS4_TX_ENDPOINT    1
#define DS4_TX_SIZE        64
#define DS4_RX_ENDPOINT    2
#define DS4_RX_SIZE        64

static const uint8_t MAX_PACKETS = 4;

namespace rds4 {
namespace ds4 {

TransportDS4Teensy *TransportDS4Teensy::inst = nullptr;
uint8_t *TransportDS4Teensy::frBuffer = nullptr;
uint32_t *TransportDS4Teensy::frSize = nullptr;

int TransportDS4Teensy::frCallbackGet(void *setup_ptr, uint8_t *data, uint32_t *len) {
    auto setup = reinterpret_cast<usb_setup_pkt_t *>(setup_ptr);
    RDS4_DBG_PRINT("TransportDS4Teensy: setupcb: Set request received type=");
    RDS4_DBG_PHEX(setup->wValue);
    RDS4_DBG_PRINT("\n");
    if (TransportDS4Teensy::inst) {
        TransportDS4Teensy::frBuffer = data;
        TransportDS4Teensy::frSize = len;
        // returns 0 on success
        return static_cast<int>(!TransportDS4Teensy::inst->onGetReport(setup->wValue, setup->wIndex, setup->wLength));
    }
    return 1;
}

int TransportDS4Teensy::frCallbackSet(void *setup_ptr, uint8_t *data) {
    auto setup = reinterpret_cast<usb_setup_pkt_t *>(setup_ptr);
    RDS4_DBG_PRINT("TransportDS4Teensy: setupcb: Get request received type=");
    RDS4_DBG_PHEX(setup->wValue);
    RDS4_DBG_PRINT("\n");
    if (TransportDS4Teensy::inst) {
        TransportDS4Teensy::frBuffer = data;
        TransportDS4Teensy::frSize = nullptr;
        // returns 0 on success
        return static_cast<int>(!TransportDS4Teensy::inst->onSetReport(setup->wValue, setup->wIndex, setup->wLength));
    }
    return 1;
}

bool TransportDS4Teensy::onGetReport(uint16_t value, uint16_t index, uint16_t length) {
    bool result = false;
    // Report error if nobody responds to the request and return true as soon as somebody responds.
    result = FeatureConfigurator<TransportDS4Teensy>::onGetReport(value, index, length) or \
             AuthenticationHandlerDS4<TransportDS4Teensy>::onGetReport(value, index, length);
    return result;
}

bool TransportDS4Teensy::onSetReport(uint16_t value, uint16_t index, uint16_t length) {
    return AuthenticationHandlerDS4<TransportDS4Teensy>::onSetReport(value, index, length);
}

bool TransportDS4Teensy::available() {
    // make sure the USB is initialized
    if (!usb_configuration) return false;
    // check for queued packets
    if (usb_rx_byte_count(DS4_RX_ENDPOINT) > 0) {
        return true;
    } else {
        return false;
    }
}

uint8_t TransportDS4Teensy::send(const void *buf, uint8_t len) {
    usb_packet_t *pkt = NULL;

    // make sure the USB is initialized
    if (!usb_configuration) return 0;
    // check for queued packets
    if (usb_tx_packet_count(DS4_TX_ENDPOINT) < MAX_PACKETS) {
        pkt = usb_malloc();
        if (pkt) {
            memcpy(pkt->buf, buf, len);
            pkt->len = len;
            usb_tx(DS4_TX_ENDPOINT, pkt);
            return len;
        } else {
            return 0;
        }
    }
    return 0;
}

uint8_t TransportDS4Teensy::sendBlocking(const void *buf, uint8_t len) {
    usb_packet_t *pkt = NULL;
    uint32_t begin = millis();

    // Blocking send
    while (1) {
        // make sure the USB is initialized
        if (!usb_configuration) return 0;
        // check for queued packets
        if (usb_tx_packet_count(DS4_TX_ENDPOINT) < MAX_PACKETS) {
            if ((pkt = usb_malloc())) {
                break;
            }
        }
        if (millis() - begin > 70) {
            RDS4_DBG_PRINTLN("send timeout");
            return 0;
        }
        // Make sure any on-yield tasks got executed during waiting
        yield();
    }
    memcpy(pkt->buf, buf, len);
    pkt->len = len;
    usb_tx(DS4_TX_ENDPOINT, pkt);
    return len;
}

uint8_t TransportDS4Teensy::recv(void *buf, uint8_t len) {
    usb_packet_t *pkt = NULL;
    uint8_t actualSize;
    if ((pkt = usb_rx(DS4_RX_ENDPOINT))) {
        // Copy at most len bytes of data. Any remaining data will be discarded
        actualSize = (pkt->len > len) ? len : pkt->len;
        memcpy(buf, pkt->buf, actualSize);
        usb_free(pkt);
        return actualSize;
    }
    return 0;
}

uint8_t TransportDS4Teensy::reply(const void *buf, uint8_t len) {
    if (TransportDS4Teensy::frBuffer and TransportDS4Teensy::frSize) {
        memcpy(TransportDS4Teensy::frBuffer, buf, len);
        (*TransportDS4Teensy::frSize) = len;
        return len;
    }
    return 0;
}

uint8_t TransportDS4Teensy::check(void *buf, uint8_t len) {
    uint8_t actual;
    if (TransportDS4Teensy::frBuffer) {
        actual = len;
        actual = actual > DS4_TX_SIZE ? DS4_TX_SIZE : actual;
        memcpy(buf, TransportDS4Teensy::frBuffer, actual);
        return actual;
    }
    return 0;
}
} // ds4
} // rds4
#endif
