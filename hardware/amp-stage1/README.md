# M2 — Headphone amplifier, stage 1 (NJM4556A)

**Status:** not started · **Hardware:** P1 → P1 + amp stage 1 · **Record results in:** `docs/build-log.md`, Milestone 2

Goal: a working headphone amplifier on the breadboard, fed from the DAC line output, from USB 5 V,
verified with a multimeter, plus (Part B) power switching and a firmware-controlled mute so it
never thumps. Background and numbers: `docs/analog-analysis.md`; design choices: `docs/architecture.md`
section 6; parts: `docs/bom.md` section B2.

- **Part A:** the amplifier itself, checked step by step on dummy loads.
- **Part B:** soft-start switched rail, mute relay on a GPIO, `amp_on()` / `amp_off()`, power budget.

How this guide works: section 2 explains each block, section 3 has values for you to work out
(post them and I'll check), section 4 is the build order. Each check in section 4 protects the
next step, so don't skip ahead.

---

## 0. Before you start

**Have:** the B2 parts, resistor assortment, multimeter, dummy loads (33 Ω and 82 Ω, ~1 W), a pair
of cheap headphones for the first listening test (not the DT770 / M40x).

**Download into `hardware/datasheets/`:** the NJM4556A datasheet and your relay's datasheet.
You'll look things up in both.

**Safety rules for this milestone**

1. Headphones only with `OUTPUT_SHIFT` ≥ 4 (−24 dB). For measurements we set it to 0 (full
   scale): then **dummy loads only**. The boot banner prints the output level; check it every time.
2. Measure before connecting: rails first, then the chip with no load, then dummy loads, then
   headphones.
3. Electrolytic capacitors have a + and a −. Backwards, they fail, sometimes violently. Check twice.
4. Power off (unplug USB) before rewiring.

---

## 1. The whole circuit at a glance

```text
                 USB 5 V (RP2040-Zero 5V pin; Part B: through the soft-start switch)
                    |
                 ferrite
                    |
          V+ ───────┼──────────────────────────────────── to NJM4556A pin 8
                    |        |              |
                 C_bulk    R_top        100 nF + 10 µF  (at pins 8 and 4)
                 470 µF      |
                    |        +──── VREF (≈ V+/2) ── C_ref 47 µF ── GND
                   GND       |
                          R_bot
                             |
                            GND

  one channel (the other is identical, using the chip's second op-amp):

  DAC L OUT ── C_in ──┬── (+) ┐
                      |       NJM4556A ──┬── R_s ──(+)C_out(−)──┬── relay ── jack TIP (L)
                R_b to VREF   (−) ───────┘                       |
                                                              R_bleed
                                                                 |
                                                                GND
  jack SLEEVE ── GND        (relay coil: GPIO → R_base → NPN → coil → 5 V, 1N4148 across the coil)
```

Pin numbers: check the NJM4556A datasheet for the DIP-8 pinout (standard dual op-amp: outputs on
1 and 7, inputs 2/3 and 6/5, V− on 4, V+ on 8). Here V− is GND.

---

## 2. What each block does, and why

**Supply filter (ferrite + C_bulk + decoupling).** The 5 V is shared with the RP2040 and the DAC.
When the amp draws a current pulse, it should come from the capacitor next to it, not through the
shared wiring, so the ferrite + 470 µF act as a small local reservoir and noise filter. The 100 nF
right at the chip's pins handles the fast stuff (an op-amp without one is a classic way to get
oscillation). See `analog-analysis.md`, "Supply bulk capacitance".

**Mid-supply reference VREF.** The op-amp has one supply (0 V and 5 V), but audio swings both
ways around zero. So we centre everything on half the supply, ≈ 2.5 V, which leaves room to swing
up and down. R_top/R_bot make it; C_ref filters supply noise out of it. VREF only biases the
inputs, which draw almost no current, so a plain divider is fine. (A divider would *not* be fine as
the headphone return; that's why the output uses capacitors to ground instead.)

**Input coupling (C_in + R_b).** The DAC output is centred on 0 V, the op-amp input must sit at
VREF. C_in passes the audio but blocks the DC difference; R_b pulls the input to VREF. Together they
form a high-pass filter: it must start well below 20 Hz. R_b also sets the input impedance the DAC
sees (requirement: ≥ 10 kΩ).

**The buffer (voltage follower).** Output wired straight back to the (−) input: gain exactly 1. The
op-amp does whatever it takes to make its output equal its (+) input, so voltage in = voltage out,
but now with tens of milliamps of drive behind it. The DAC has enough voltage; it's the current it
can't supply (`analog-analysis.md`, "How much output is needed").

**Output (R_s + C_out + R_bleed).** The op-amp output sits at ≈ 2.5 V DC; headphones must see 0 V
DC. C_out blocks the DC; with the headphone it forms another high-pass filter, and because a
headphone is only 32–80 Ω, the capacitor has to be big. R_s, a few ohms, isolates the op-amp from
the cable's capacitance (helps stability) and limits current into a short. R_bleed keeps the
jack side of C_out at 0 V, so plugging in headphones doesn't click, and it is the path through which
C_out charges at power-up.

**Mute relay.** At power-up C_out charges from 0 to 2.5 V. Through a headphone that's a thump.
The relay stays open while C_out charges through R_bleed, then closes. In Part A it's simply
bridged with a wire and you test on dummy loads; Part B puts it under firmware control.

---

## 3. Values to work out

Formula you need for the filters: a resistor R and capacitor C form a high-pass (or low-pass) with
corner frequency **f = 1 / (2π·R·C)**. An RC charges to ~99 % in about **5·R·C**.

Post your answers (with how you got them); I'll check before you build.

| # | Question | Hints |
|---|---|---|
| Q1 | Output high-pass: f with C_out = 1000 µF into 32 Ω and into 80 Ω (take R_s as 4.7 Ω). And with 470 µF into 32 Ω? | R is R_s + headphone |
| Q2 | Choose R_b and C_in so the input high-pass is below ~3 Hz and the input impedance ≥ 10 kΩ. | Large R_b → small C_in; but R_b × the op-amp's input bias current (datasheet) is an offset voltage |
| Q3 | Choose R_top = R_bot. What's the VREF filter corner with C_ref = 47 µF? Roughly how long until VREF settles after power-up? | For the filter, R is R_top ∥ R_bot. 10 k–100 k is sensible |
| Q4 | Choose R_bleed so C_out (1000 µF) is charged within ~3 s. How much does R_bleed load the op-amp compared with a 32 Ω headphone? | 5·R·C. This becomes the firmware's unmute delay in Part B |
| Q5 | Relay coil current from the relay datasheet (5 V / coil resistance). Choose R_base so the NPN is fully on from a 3.3 V GPIO. | Aim base current ≈ coil current / 10 (forced β of 10). V_BE ≈ 0.7 V |
| Q6 | From the NJM4556A datasheet, estimate the maximum clean output swing on a 5 V supply into 32 Ω. At what dBFS do you expect clipping to start? | Full scale = 2.1 Vrms. Look for "maximum output voltage" and the load it's specified at; it's given for ± supplies |
| Q7 | Datasheet lookup: supply current (both channels, no load), minimum supply voltage, and whether it's stable as a voltage follower. | You'll compare the supply current with your measurement in step A4 |

---

## 4. Part A — build and check

Build **one channel first**, check it, then copy it.

**A1. Supply only.** Wire ferrite, C_bulk, decoupling and the VREF divider + C_ref. No chip in the
socket. *Plug USB in first, then connect the amp supply wire*, because 470 µF straight on VBUS
makes a big inrush at plug-in (Part B's soft-start fixes this properly).
Measure: V+ (should be the USB voltage, ~5 V), VREF (≈ half of V+). Note both.

**A2. Chip in, no load, no signal.** Put the multimeter **in series** with the amp supply (current
range, mA). Insert the NJM4556A, power up.
Measure: supply current. Compare with Q7. **Much higher than the datasheet, a warm chip, or a
reading that changes when you touch wires = oscillation.** Stop and fix before going on (shorter
wires, decoupling closer to the pins, check R_s).
Then: DC at the op-amp output (≈ VREF?) and DC across the output after C_out (should be ~0 mV
once C_out has charged).

**A3. Dummy load, no signal.** 33 Ω on the output (relay bridged). Supply current and output DC
again: should barely change. Wait for C_out to charge, then DC offset across the load: a few mV
at most.

**A4. Signal, dummy load.** Flash firmware with `OUTPUT_SHIFT 0` (edit `main.c`, *don't commit it*);
the banner must say `output 0 dB`. Make the test tones:

```sh
python3 hardware/amp-stage1/tools/make_tones.py           # 1 kHz, 0 to −20 dBFS
python3 hardware/amp-stage1/tools/make_tones.py --freq 400  # if the meter's AC range is poor at 1 kHz
afplay tones/tone_1000Hz_-20dBFS.wav
```

Mac output = signaldeck, Mac volume at maximum. Starting at −20 dBFS and going up 1 dB at a time,
read AC volts across the 33 Ω load. Expected: 2.1 Vrms × 10^(dBFS/20), so each 6 dB step halves
or doubles it. Write a table. Where the reading stops following, that's clipping. Compare with Q6.
Repeat at 82 Ω. Also note the supply current at the loudest clean level.

**A5. Second channel.** Build it, repeat A2–A4 quickly, check L and R match.

**A6. Listening.** Back to `OUTPUT_SHIFT 4` (banner: `output -24 dB`). Cheap headphones first,
then yours. Music, then silence at max Mac volume: listen for hum and hiss. Power the amp supply
on/off and hear the thump that Part B will remove.

**A7. 16 Ω check (optional).** Two 33 Ω in parallel ≈ 16 Ω as a dummy load, at the clipping level
from A4: the output should clip lower, the chip may get warm, nothing should fail. Keep it short.

Record everything in `docs/build-log.md`, Milestone 2.

---

## 5. Part B — switching and mute (outline)

Details once Part A works. What it covers:

1. **Soft-start switch.** Pick a P-MOSFET (or a load-switch IC) whose on-resistance is specified at
   V_GS = −4.5 V or lower, driven through an NPN so the gate swings the full 5 V, with an RC on the
   gate so it turns on over milliseconds. Through-hole parts that suit are rare; a SOT-23 part on
   an adapter board is fine.
2. **GPIO assignment.** AUDIO_EN (switch) and AMP_MUTE (relay). Choose free pins, record them in
   `docs/pinout.md`.
3. **Firmware** (`firmware/rp2040`): `amp_on()` = rail on → wait for VREF and C_out (Q3/Q4) →
   relay closed; `amp_off()` = relay open → short wait → rail off. Called at USB mount/unmount and
   suspend/resume.
4. **Unplug race.** When USB is pulled, there's no firmware time. Check whether the relay drops out
   before C_out discharges into the headphone; if not, find a fix.
5. **Power budget** at idle and at loud levels → set `bMaxPower`, firmware version 0.2.0.

---

## 6. Results (fill in)

| Check | Expected | Measured |
|---|---|---|
| V+ / VREF | ~5 V / ~2.5 V | |
| Supply current, no load | Q7 | |
| DC offset at load, L / R | < few mV | |
| Clipping starts, 33 Ω / 82 Ω | Q6 | |
| Max clean output, 33 Ω / 82 Ω | ~1 Vrms | |
| Supply current, loud | | |
| Hum / hiss at max | inaudible | |
