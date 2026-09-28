# Design Decisions

This file records decisions so that discussions, firmware changes, and hardware revisions do not silently change the project architecture.

Statuses:

- **Accepted** - current design direction
- **Prototype** - current experimental choice; may change
- **TBD** - intentionally undecided
- **Deferred** - not part of the current milestone

| Area | Decision | Status | Rationale |
|---|---|---|---|
| MCU | Waveshare RP2040-Zero | Accepted | USB-C, RP2040 ecosystem, compact, programmable and already suitable for experimentation |
| External connection | Single USB-C for audio + power | Accepted | Simple desktop device with one cable |
| USB audio | USB Audio Class | Accepted | Standard host audio-device behavior |
| Initial format | Stereo 24-bit / 48 kHz | Prototype | Keep firmware/debugging manageable before increasing sample rate |
| DAC | PCM5102A module | Prototype | Simple I2S DAC, inexpensive and modular |
| DAC clocking | Prefer configuration without external MCLK | Prototype | Reduces RP2040 clocking complexity |
| Volume | Analog, 10 kOhm dual logarithmic | Prototype | Simple tactile control independent of host software |
| Headphone outputs | 3.5 mm + 6.35 mm TRS | Accepted | Primary product requirement |
| Headphone amplifier | Not yet selected | TBD | Must be chosen together with power topology and load requirements |
| Initial headphone load target | Approximately 16-80 Ohm | Prototype | Covers common portable/studio headphones while keeping USB-powered design realistic |
| Level indication | Two physical L/R analog meters | Accepted | Functional level display and core industrial/instrument aesthetic |
| Meter signal | Tap before volume control | Prototype | Meter activity independent of listening volume |
| Status indicators | USB + AUDIO | Prototype | Avoid unnecessary front-panel clutter |
| Power switch | Switch analog/audio subsystem while MCU can remain powered | Prototype | Keeps USB device enumerated while audio section is off |
| Mechanical design | Two metal front subpanels | Accepted | Aircraft/instrument-panel visual language |
| Leather | Red leather as enclosure/accent material | Prototype | Reuse existing material without compromising control mounting |
| Microphone input | Reserve as future expansion | Deferred | Useful feature but adds ADC, preamp and USB capture complexity |
| Custom PCB | After modular prototype works | Deferred | Avoid debugging USB, DAC, analog and PCB layout simultaneously |
