/* SPDX-License-Identifier: Unlicense */

#include <RDS4-DS4.hpp>
#include <PS4USB.h>

// Namespace aliases
namespace rds4api = rds4::api;
namespace ds4 = rds4::ds4;

// Setup USBH, which is required by rds4::ds4::AuthenticatorUSBH
USB USBH;

// Use patched PS4USB instead of the stock one, note that only patched PS4USB works with rds4::ds4::AuthenticatorUSBH
ds4::PS4USB2 RealDS4(&USBH);

// Setup the authenticator
ds4::AuthenticatorUSBH RealDS4Au(&RealDS4);

// Setup the transport backend and register the authenticator to it
ds4::TransportTeensy DS4Tr(&RealDS4Au);

// Setup the DS4 object using the transport backend
ds4::Controller DS4(&DS4Tr);

void setup() {
    Serial1.begin(115200);
    Serial1.println("Hello");
    if (USBH.Init() == -1) {
        Serial.println("USBH init failed");
        while (1);
    }
    DS4.begin();
}

void loop() {
    // Poll the USB host controller
    USBH.Task();
    // Handle any updates on auth
    DS4Tr.update();
    // Grab updates from the host (e.g. rumble and LED state)
    DS4.update();
    // Send DS4 report if possible
    DS4.sendReport();
}
