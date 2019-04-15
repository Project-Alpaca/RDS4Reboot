/* SPDX-License-Identifier: Unlicense */

#include <AuthenticatorDS4.hpp>
#include <ControllerDS4.hpp>
#include <TransportDS4.hpp>
#include <PS4USB.h>

using namespace rds4;

// Setup USBH, which is required by rds4::AuthenticatorDS4USBH
USB USBH;
PS4USB2 RealDS4(&USBH);

// Setup the authenticator
AuthenticatorDS4USBH RealDS4Au(&RealDS4);

// Setup the transport backend and register the authenticator to it
TransportDS4Teensy DS4Tr(&RealDS4Au);

// Setup the DS4 object using the transport backend
ControllerDS4 DS4(&DS4Tr);

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
