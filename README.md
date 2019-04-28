# RDS4Reboot

A (hopefully) better game controller library for current-gen game consoles.

**WARNING**: Work in progress and not ready for general use. API may change at any time.

## Disclaimer

This project is for research purposes only and does not come with any warranty. Use it at your own risk.

## Why not b***k?

Simply put, existing software/boards like B\*\*\*k, C\*\*\*\*\*u, P\*\*\*0, etc. are either not flexible enough for my needs (I want full control over what the controller sends and receives, not just buttons and sticks), proprietary as f\*\*\*, does not support current-gen consoles because they have "impossible to crack security measures", too expensive, or all of the above. Besides that, I am not hardcore enough to think that scrapping a $60 controller and figuring out how to padhack the touchpad, gyroscope, accelerometer, rumble motor, etc. without running into random obstacles such as frying the motherboard would be a good idea. So here we are.

## Use cases

- Fightsticks
- Rhythm game controllers
- Accessible controllers
- Creative projects (e.g. beat <strike>Dark Souls</strike> <strike>BloodBorne</strike> Sekiro with bananas, etc.)

## Current status

### Works

- Builds on Teensy LC
- USB Host Shield authenticator with
  - Official controller
  - Hori Mini
- Authentication over USB Host Shield authenticator and Teensy USB transport
- Interrupt IN/OUT reports over Teensy USB transport

### Not (throughly) tested

- Example sketch(es)
- Feedback report handling
- Input report manipulation
- Teensy transport (needs more testing)

### Does not work/WIP

- PluggableUSB transport
- UnoJoy compatibility layer
- Authentication using Guitar Hero Dongle (needs hack)
- Per-project library configuration
- More example sketches

### Planned

- A7105 authenticator
