# Datasheets

The PDFs are kept locally and **not committed** (`.gitignore`): they are the manufacturers'
copyrighted documents and this repository is public. This list records which document and revision
the design was based on and where to get it, so a fresh clone can rebuild the folder.

Rule: the datasheet of the **exact part number bought**, from the manufacturer.

| Part | Local file | Revision / date | Source | Used for |
|---|---|---|---|---|
| RP2040 | `RP-008371-DS-1-rp2040-datasheet.pdf` | build date 2025-02-20 | [raspberrypi.com](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf) | firmware, C labs (section numbers refer to this build) |
| PCM5102A | `pcm5102a.pdf` | SLAS859C | [ti.com](https://www.ti.com/lit/ds/symlink/pcm5102a.pdf) | DAC output level, load |
| NJM4556A | `NJM4556A_E-1917507.pdf` | doc E-1917507 | [nisshinbo-microdevices.co.jp](https://www.nisshinbo-microdevices.co.jp/en/products/operational-amplifier/spec/?product=njm4556a) | M2 amplifier |

To add (M2): the mute relay model you buy; Waveshare RP2040-Zero pinout (wiki page).
Later: jacks / encoder / toggle drawings (M3); OPA1622, LM27762, TIDA-01341 (M4).
