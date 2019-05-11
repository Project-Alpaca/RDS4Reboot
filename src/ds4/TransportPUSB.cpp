// SPDX-License-Identifier: LGPL-3.0-or-later
/** TransportDS4PUSB.cpp
 *  PluggableUSB transport back-ends for PS4 controllers.
 *
 *  Copyright 2019 dogtopus
 */

#include "Transport.hpp"
#include "utils/utils.hpp"

#if defined(RDS4_ARDUINO) && defined(USBCON)

#if defined(ARDUINO_ARCH_SAMD)
// TODO USB API binding for SAMD
typedef uint32_t usb_ep_t;
// TODO is the return value really uint8_t?
static inline uint8_t USB_Send(usb_ep_t ep, void *buf, uint8_t len) {
    return USBDevice.send(ep, buf, len);
}
static inline uint8_t USB_ClearToSend(usb_ep_t ep) {
    //TODO
    return 0;
}
#else
typedef uint8_t usb_ep_t;
static inline uint8_t USB_ClearToSend(usb_ep_t ep) {
    return USB_SendSpace(ep);
}
#endif

uint8_t TransportDS4PUSB::send(const void *buf, uint8_t len) {
    // Use USB_SendSpace() to check if we can send data
    // On SAMD this should be done by checking DMA buffer readiness and transfer status.
    uint8_t space = USB_ClearToSend(this->getOutEP());
    if (space == 0) {
        
        USB_Send(this->getInEP(), buf, len);
    } else {
        return 0;
    }
}

uint8_t TransportDS4PUSB::recv(void *buf, uint8_t len) {
    //return USB_Recv(this->getOutEP(), buf, len);
}


bool TransportDS4PUSB::available() {
    //return USB_Available(this->getOutEP());
}

#endif // defined(RDS4_ARDUINO) && defined(USBCON)
