#!/usr/bin/env python3
"""Make test-tone WAV files for the M2 amplifier checks.

Writes 48 kHz / 24-bit stereo sine files (the format signaldeck plays natively), one per level:
    tones/tone_1000Hz_-06dBFS.wav, ...

Usage:
    python3 make_tones.py                    # 1000 Hz, 0 to -20 dBFS in 1 dB steps, 20 s each
    python3 make_tones.py --freq 400         # if your multimeter's AC range is only good to ~400 Hz
    python3 make_tones.py --levels 0 -3 -6 --seconds 10

Play one on the Mac with:  afplay tones/tone_1000Hz_-06dBFS.wav
(select "signaldeck USB DAC" as the output and set the Mac volume to maximum first;
the OS slider is fixed until the Feature Unit exists, which is what we want here).

Standard library only.
"""
import argparse
import math
import os
import struct
import wave

RATE = 48000
FULL_SCALE = 2**23 - 1          # largest positive 24-bit sample


def write_tone(path, freq, dbfs, seconds):
    amp = FULL_SCALE * 10 ** (dbfs / 20)
    n = int(RATE * seconds)
    fade = int(RATE * 0.02)     # 20 ms fade in/out, so the file itself doesn't click
    frames = bytearray()
    for i in range(n):
        g = min(1.0, i / fade, (n - 1 - i) / fade)
        s = int(round(amp * g * math.sin(2 * math.pi * freq * i / RATE)))
        b = struct.pack("<i", s)[:3]    # little-endian 24-bit: low 3 bytes of a 32-bit int
        frames += b + b                 # same sample on left and right
    with wave.open(path, "wb") as w:
        w.setnchannels(2)
        w.setsampwidth(3)
        w.setframerate(RATE)
        w.writeframes(bytes(frames))


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--freq", type=float, default=1000)
    p.add_argument("--levels", type=float, nargs="+", default=[-i for i in range(21)],
                   help="levels in dBFS (0 = full scale)")
    p.add_argument("--seconds", type=float, default=20)
    p.add_argument("--out", default="tones")
    a = p.parse_args()

    os.makedirs(a.out, exist_ok=True)
    for lvl in a.levels:
        name = f"tone_{a.freq:g}Hz_{lvl:+03.0f}dBFS.wav"     # e.g. tone_1000Hz_-06dBFS.wav
        path = os.path.join(a.out, name)
        write_tone(path, a.freq, lvl, a.seconds)
        print(path)


if __name__ == "__main__":
    main()
