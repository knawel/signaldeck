# Versioning

Three independent numbers, so none of them can be mistaken for another.

| What | Scheme | Current | Bumped when |
|---|---|---|---|
| Product generation | `V1`, `V2` | V1 (in progress) | The feature set changes (README "Target features"; V2 = mic/ADC) |
| Hardware revision | `P1`, `P2`, … for hand-built prototypes; `A`, `B`, … for PCBs | **P1** | Any wiring or pinout change that firmware or someone rebuilding it needs to know about |
| Firmware | SemVer `MAJOR.MINOR.PATCH`, `0.x` until V1 is complete | **0.1.0** | MINOR: new feature (encoder, meters, amp control, …). PATCH: a fix. MAJOR: 1.0.0 = V1 complete |

## Hardware revisions

| Rev | Build | Notes |
|---|---|---|
| P1 | Breadboard: RP2040-Zero + PCM5102A module, GP0–2 I2S, DAC VIN from board 5V, XSMT at 3V3 | Pinout in `pinout.md`, as of 2026-11-15 |

Add a row (and update `pinout.md`) when the next revision is wired. Purely mechanical changes
that don't alter any connection don't need a new revision.

## Firmware

- **Single source:** `project(signaldeck VERSION x.y.z)` in `firmware/rp2040/CMakeLists.txt`, next to
  `SIGNALDECK_HW_REV`. CMake passes both to the code; nothing else holds a version number.
- It appears in three places:
  - USB `bcdDevice` (`0xJJMN`: 0.1.0 → `0x0010`, which macOS shows as "0.10")
  - the serial port banner at boot
  - `picotool info`
- Constraint from `bcdDevice`: minor and patch must stay 0–9 (a `_Static_assert` stops the build
  otherwise). After `0.9.x` comes `0.10`, which won't fit, so go to `1.0.0` or rethink the scheme then.
- Each firmware release states the hardware revisions it supports. A firmware build targets
  one revision via `SIGNALDECK_HW_REV`. On a PCB, 2–3 strap pins could let it detect the revision.

## Releasing

1. Bump the version in `CMakeLists.txt` (and `SIGNALDECK_HW_REV` if the hardware changed).
2. Build, flash, check the banner, then commit.
3. Tag the commit: `git tag -a fw/vX.Y.Z -m "..."` and, for a new hardware revision, `git tag -a hw/Pn`.
4. Add a row below.

| Firmware | Hardware | Date | Summary |
|---|---|---|---|
| 0.1.0 | P1 | 2026-11-15 | First working USB DAC: UAC2 48 kHz/24-bit, async feedback (P controller), fixed −24 dB |
