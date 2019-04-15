# RDS4Reboot

A (hopefully) better game controller library for current-gen game consoles.

**WARNING**: Work in progress and not ready for general use. API may change at any time.

## Disclaimer

This project is for research purposes only and does not come with any warranty. Use it at your own risk.

## Current status

### Works

- Builds on Teensy LC
- USB Host Shield authenticator with
  - Official controller
  - Hori Mini
- Authentication over USB Host Shield authenticator and Teensy USB transport
- Report sending/receiving over Teensy USB transport

### Not (throughly) tested

- Example sketch(es)
- Feedback report handling
- Input sending
- Teensy transport (needs more testing)

### Does not work/WIP

- PluggableUSB transport
- UnoJoy compatibility layer
- Authentication using Guitar Hero Dongle (needs hack)
- Per-project library configuration

### Planned

- A7105 authenticator
