// SPDX-License-Identifier: LGPL-3.0-or-later
/** TransportDS4.cpp
 *  Various transport back-ends for PS4 controllers.
 *
 *  Copyright 2019 dogtopus
 */

#include "Transport.hpp"
#include "utils/utils.hpp"

#if defined(RDS4_ARDUINO) && defined(RDS4_TEENSY_3)

#include <usb_dev.h>
#include <usb_ds4stub.h>

// These should be in sync with the DS4 stub
static const uint8_t TX_ENDPOINT = 1;
static const uint8_t TX_SIZE = 64;
static const uint8_t RX_ENDPOINT = 2;

static const uint8_t MAX_PACKETS = 2;

namespace rds4 {
namespace ds4 {

TransportTeensy *TransportTeensy::inst = nullptr;
uint8_t *TransportTeensy::frBuffer = nullptr;
uint32_t *TransportTeensy::frSize = nullptr;

int TransportTeensy::frCallbackGet(void *setup_ptr, uint8_t *data, uint32_t *len) {
    auto setup = reinterpret_cast<usb_setup_pkt_t *>(setup_ptr);
    RDS4_DBG_PRINT("TransportDS4Teensy: setupcb: Set request received type=");
    RDS4_DBG_PHEX(setup->wValue);
    RDS4_DBG_PRINT("\n");
    if (TransportTeensy::inst) {
        TransportTeensy::frBuffer = data;
        TransportTeensy::frSize = len;
        // returns 0 on success
        return static_cast<int>(!TransportTeensy::inst->onGetReport(setup->wValue, setup->wIndex, setup->wLength));
    }
    return 1;
}

int TransportTeensy::frCallbackSet(void *setup_ptr, uint8_t *data) {
    auto setup = reinterpret_cast<usb_setup_pkt_t *>(setup_ptr);
    RDS4_DBG_PRINT("TransportDS4Teensy: setupcb: Get request received type=");
    RDS4_DBG_PHEX(setup->wValue);
    RDS4_DBG_PRINT("\n");
    if (TransportTeensy::inst) {
        TransportTeensy::frBuffer = data;
        TransportTeensy::frSize = nullptr;
        // returns 0 on success
        return static_cast<int>(!TransportTeensy::inst->onSetReport(setup->wValue, setup->wIndex, setup->wLength));
    }
    return 1;
}

bool TransportTeensy::onGetReport(uint16_t value, uint16_t index, uint16_t length) {
    bool result = false;
    // Report error if nobody responds to the request and return true as soon as somebody responds.
    result = FeatureConfigurator<TransportTeensy>::onGetReport(value, index, length) or \
             AuthenticationHandler<TransportTeensy>::onGetReport(value, index, length);
    return result;
}

bool TransportTeensy::onSetReport(uint16_t value, uint16_t index, uint16_t length) {
    return AuthenticationHandler<TransportTeensy>::onSetReport(value, index, length);
}

bool TransportTeensy::available() {
    // make sure the USB is initialized
    if (!usb_configuration) return false;
    // check for queued packets
    if (usb_rx_byte_count(RX_ENDPOINT) > 0) {
        return true;
    } else {
        return false;
    }
}

uint8_t TransportTeensy::send(const void *buf, uint8_t len) {
    usb_packet_t *pkt = NULL;

    // make sure the USB is initialized
    if (!usb_configuration) return 0;
    // check for queued packets
    if (usb_tx_packet_count(TX_ENDPOINT) < MAX_PACKETS) {
        pkt = usb_malloc();
        if (pkt) {
            memcpy(pkt->buf, buf, len);
            pkt->len = len;
            usb_tx(TX_ENDPOINT, pkt);
            return len;
        } else {
            return 0;
        }
    }
    return 0;
}

uint8_t TransportTeensy::sendBlocking(const void *buf, uint8_t len) {
    usb_packet_t *pkt = NULL;
    uint32_t begin = millis();

    // Blocking send
    while (1) {
        // make sure the USB is initialized
        if (!usb_configuration) return 0;
        // check for queued packets
        if (usb_tx_packet_count(TX_ENDPOINT) < MAX_PACKETS) {
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
    usb_tx(TX_ENDPOINT, pkt);
    return len;
}

uint8_t TransportTeensy::recv(void *buf, uint8_t len) {
    usb_packet_t *pkt = NULL;
    uint8_t actualSize;
    if ((pkt = usb_rx(RX_ENDPOINT))) {
        // Copy at most len bytes of data. Any remaining data will be discarded
        actualSize = (pkt->len > len) ? len : pkt->len;
        memcpy(buf, pkt->buf, actualSize);
        usb_free(pkt);
        return actualSize;
    }
    return 0;
}

uint8_t TransportTeensy::setOutgoingFeatureReport(const void *buf, uint8_t len) {
    if (TransportTeensy::frBuffer and TransportTeensy::frSize) {
        memcpy(TransportTeensy::frBuffer, buf, len);
        (*TransportTeensy::frSize) = len;
        return len;
    }
    return 0;
}

uint8_t TransportTeensy::getIncomingFeatureReport(void *buf, uint8_t len) {
    uint8_t actual;
    if (TransportTeensy::frBuffer) {
        actual = len;
        actual = actual > TX_SIZE ? TX_SIZE : actual;
        memcpy(buf, TransportTeensy::frBuffer, actual);
        return actual;
    }
    return 0;
}
} // ds4
} // rds4
#endif
