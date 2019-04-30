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

## Usage

Currently the library only supports Teensy 3.x (including LC). A patched version of Teensyduino core (version 1.45 at the moment) is required, which can be found [here][td-ds4].

For Teensyduino IDE users, there are two ways to install the patched library. One way is to run `scripts/makepatch.sh` and apply the generated patch file using the `patch` tool to an existing Teensyduino core installation. The other way is to directly replace the core library of a working installation with the patched version. Then you might want to edit the `boards.txt` in a similar fashion as [this][td-boards] (with necessary changes like replace `USB_XINPUT` with `USB_DS4STUB` and so on). For PlatformIO users, it is possible to use the companion script hosted [here][td-pfio] and follow the instruction [here][td-pfio-readme]. Afterwards add `-DUSB_DS4STUB -UUSB_SERIAL -DCORE_TEENSY -D_ARM_` to the build parameters.

After patching the Teensyduino core library just download and install RDS4Reboot normally.

## Current status

### Works

- Builds on Teensy 3.6 and LC
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
- More example sketches

### Planned

- A7105 authenticator

[td-ds4]: https://github.com/dogtopus/teensy-cores
[td-boards]: https://github.com/zlittell/MSF-XINPUT/blob/master/MSF_XINPUT/Teensyduino%20Files%20that%20were%20edited/hardware/teensy/avr/boards.txt#L833
[td-pfio]: https://github.com/dogtopus/FT-Controller-FW/tree/master/patches
[td-pfio-readme]: https://github.com/dogtopus/FT-Controller-FW#build
