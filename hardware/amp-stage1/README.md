# M2 — Headphone amplifier, stage 1 (NJM4556A)

**Status:** not started · **Hardware:** P1 → P1 + amp stage 1 · **Record results in:** `docs/build-log.md`, Milestone 2

Goal: a working headphone amplifier on the breadboard, fed from the DAC line output, from USB 5 V,
verified with a multimeter, plus (Part B) power switching and a firmware-controlled mute so it
never thumps. Background and numbers: `docs/analog-analysis.md`; design choices: `docs/architecture.md`
section 6; parts: `docs/bom.md` section B2.

- **Part A:** the amplifier itself, checked step by step on dummy loads.
- **Part B:** soft-start switched rail, mute relay on a GPIO, `amp_on()` / `amp_off()`, power budget.

How this guide works: section 2 explains each block, section 3 gives the background (school-level
physics is enough) and the values for you to work out (post them and I'll check), section 4 is the build order. Each check in section 4 protects the
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

![Amp stage 1 schematic](schematic.svg)

(Drawn, with the wiring view below, by `tools/draw_schematic.py`, which needs `pip install schemdraw`. The names without values,
such as R_b and R_bleed, are the ones you work out in section 3. The same circuit in plain text:)

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

### Wiring view: what goes on each U1 pin

The schematic shows how the circuit works; this view is for wiring the breadboard. U1 is the
NJM4556AM (surface-mount DMP8) soldered on a SOP-8 to DIP-8 adapter, so on the breadboard it
behaves like a DIP-8 chip with the same pin numbers. The view shows it from the top, pin 1 at the
top left. Check the pinout against the datasheet.

**Mounting U1 on the adapter (once, before step A2).** The chip's pin 1 has a dot or a bevelled
edge; line it up with the pin 1 mark on the adapter (adapters map pad n to DIP pin n, but check
yours). Flux the pads, tack one corner pin, check the alignment of all eight pins, then solder the
rest. Afterwards check with the multimeter (continuity mode) that each chip pin reaches its DIP pin
and that no two neighbouring pins are bridged.

![U1 wiring view](wiring.svg)

Use the table as a checklist before the first power-up (step A2):

| Pin | Name | Connect to |
|---|---|---|
| 1 | OUT A | R_s (L) → C_out (L) → relay → jack TIP; and pin 2 |
| 2 | −IN A | pin 1 (follower jumper) |
| 3 | +IN A | C_in (L) from DAC L OUT; R_b (L) to VREF |
| 4 | V− | GND; − side of the 100 nF and 10 µF |
| 5 | +IN B | C_in (R) from DAC R OUT; R_b (R) to VREF |
| 6 | −IN B | pin 7 (follower jumper) |
| 7 | OUT B | R_s (R) → C_out (R) → relay → jack RING; and pin 6 |
| 8 | V+ | V+ (filtered, after the ferrite); + side of the 100 nF and 10 µF |

Pin numbers: check the NJM4556A datasheet for the pinout (standard dual op-amp: outputs on
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

## 3. Background, then the values to work out

### 3.1 The ideas you need

Seven short ideas, each with a worked example that uses *different* numbers from the questions.
Keep a calculator handy and convert units first: µF = 10⁻⁶ F, kΩ = 10³ Ω, mA = 10⁻³ A.

**1. Ohm's law and power.** V = I·R, and power P = V·I = V²/R.
*Example:* 2 V across a 100 Ω resistor → I = 2 / 100 = 20 mA, P = 2 × 0.02 = 40 mW.

**2. RMS and peak.** An AC voltage goes up and down; a multimeter on "V~" shows the **RMS** value,
a kind of average that gives the same heating as DC. For a sine: peak = RMS × 1.414, and it swings
from −peak to +peak.
*Example:* 1 Vrms → 1.41 V peak → 2.83 V from bottom to top.

**3. Decibels.** A ratio on a log scale. For voltages: dB = 20·log₁₀(V / V_ref), and back:
V = V_ref × 10^(dB/20). Useful anchors: −6 dB ≈ half, −20 dB = one tenth. **dBFS** means "relative
to digital full scale": 0 dBFS is the loudest sample the DAC can play, 2.1 Vrms here.
*Example:* −12 dBFS → 2.1 × 10^(−12/20) = 2.1 × 0.25 = 0.53 Vrms.

**4. Capacitors.** A capacitor stores charge. With DC, current flows only until it's charged, then
stops, so it **blocks DC**. With AC it keeps charging and discharging, so AC **passes**, and the
easier the higher the frequency or the bigger the capacitor. Its "resistance to AC" (reactance) is
X_C = 1 / (2π·f·C).

A capacitor in series with the signal and a resistor after it (to ground or to VREF) makes a
**high-pass filter**: low frequencies are lost across the capacitor, high ones pass. The **corner
frequency** is where X_C equals R:

&nbsp;&nbsp;&nbsp;&nbsp;**f = 1 / (2π·R·C)**

At f the signal is already 3 dB down (about 70 %), at 2·f about 1 dB down, at 4·f almost nothing.
For audio we want the corner well below 20 Hz.
*Example:* C = 10 µF, R = 1 kΩ → f = 1 / (2π × 1000 × 0.00001) = 15.9 Hz.
Rearranged to find C for a wanted corner: C = 1 / (2π·R·f).

**5. RC charging time.** The **time constant** τ = R·C (seconds, with ohms and farads). A
capacitor charging through a resistor reaches 63 % after τ and is practically full (99 %) after 5τ.
*Example:* 10 µF through 1 kΩ: τ = 10 ms, full after ~50 ms.
**Electrolytic** capacitors have polarity: the + leg must be at the higher DC voltage.

**6. The op-amp, as used here.** An amplifier with enormous gain. With negative feedback (output
connected back to the (−) input), two rules describe it well:
(a) the output does whatever it takes to make (−) equal (+);
(b) the inputs draw (almost) no current.
In a **follower**, (−) is wired to the output, so by rule (a) output = (+) input: gain 1.
Real limits, which the datasheet gives: the output can't reach the supply rails (it stops a volt or
more short, more with a heavy load), the output current is limited, and a tiny **input bias
current** (nanoamps) does flow into the inputs.

**7. The transistor as a switch, and relays.** In an NPN transistor a small current into the base
lets a big current flow from collector to emitter. To be sure it's **fully on** (a closed switch,
"saturated"), give it a base current of about a tenth of the collector current. The base-emitter
junction drops about 0.7 V, so the base resistor is R = (V_drive − 0.7) / I_base.
*Example:* switch 100 mA from a 5 V logic pin: I_base = 10 mA, R = (5 − 0.7) / 0.01 = 430 Ω.
A relay coil is an electromagnet. When its current is switched off, it produces a voltage spike that
can kill the transistor; the **diode** across the coil gives that current a harmless path.

**Reading datasheet tables.** Values come as Min / Typ / Max, *with test conditions* (supply
voltage, load, temperature). Always compare the conditions with your own. Many op-amp tables are
for ±15 V supplies; our "±2.5 V" (0 and 5 V with the middle at 2.5 V) is much less, so look for
graphs against supply voltage, or estimate.

### 3.2 The questions

Post your answers with the working; wrong answers are useful too, they show where to explain more.
I'll check everything before you build.

**Q1 — Output high-pass (C_out with the headphone).**
a) The resistance after C_out is R_s + headphone. With R_s = 4.7 Ω and a 32 Ω headphone, R = 36.7Ω
b) Corner frequency with C_out = 1000 µF. 4.3 Hz
c) The same for the 80 Ω DT770. 1.9 Hz
d) The same for 470 µF and 32 Ω. 9.2 Hz
e) Are these fine for audio? (Idea 4: what happens at 20 Hz if the corner is at f?) yes

**Q2 — Input coupling (C_in and R_b).**
a) The DAC "sees" R_b as its load. The requirement is ≥ 10 kΩ. Take R_b = 100 kΩ.
b) Find C_in for a corner at or below 3 Hz (rearranged formula, idea 4). 531 nF
c) Pick the nearest value from the BOM range (1–2.2 µF film). What corner does it give? 1uF, it gives 1.6 Hz
d) Look up the NJM4556A input bias current (Q7). Bias current × R_b = a small DC voltage error at
   the input. How big? Does it matter? (Hint: does DC reach the headphone? Look at C_out.) 50nA, so it gives 5mV. It doesn't matter, because C_out block DC.

**Q3 — The mid-supply reference VREF.**
a) Why do two equal resistors give VREF = V+/2? Because I is the same and V1 = V2 = IR1 = IR2
b) Choose a value between 10 kΩ and 100 kΩ. How much current flows through the divider? 250uA for R=10kΩ
c) For the filter, the resistance C_ref "sees" is both resistors in parallel: R_top·R_bot /
   (R_top + R_bot), which for equal resistors is half of one. Value? 5kΩ
d) Corner frequency with C_ref = 47 µF. (This is a *low-pass*: it lets DC through and removes
   the supply noise above the corner. Same formula.) 0.68Hz
e) Time until VREF has settled after power-up (idea 5). 99% will be at 5t. t = RC = 0.235sec, so 5t is 1.175sec

**Q4 — R_bleed and the unmute delay.**
a) C_out (1000 µF) charges through R_bleed. For "charged within 3 s", what's the largest τ
   allowed (idea 5)? And so the largest R_bleed? To have 99%, τ=0.6s, so R = 600Ω
b) Pick a standard value at or below that. closest is 560Ω
c) R_bleed is in parallel with the headphone. At 1 Vrms, how much current does it take, compared
   with the 32 Ω headphone's current? Is it a problem? I_phone/I_bleed = R_bleed/R_phone = 17.5. It is not a problem

**Q5 — Relay driver.** (Needs your relay's datasheet.)
a) Find the coil resistance (or nominal coil current) for the 5 V version. Coil current = 5 V /
   coil resistance. 178Ω, so I  = 28mA
b) Base current for "fully on" (idea 7). So we need to have 1/10th of coil current to be fully on, so 2.8mA
c) R_base from a 3.3 V GPIO; pick the nearest standard value *below* (more base current is safe). Ideal is (3.3 - 0.7)/(2.8mA) = 928Ω, so 820Ω
d) An RP2040 GPIO delivers 4 mA by default (12 mA max if configured). Enough? Yes
e) In one sentence: what does the 1N4148 across the coil do? When device is switch off, coil will produce electrical spike, and diod pass that spike back

**Q6 — Where will it clip?**
a) Full scale is 2.1 Vrms. Peak voltage? 2.1*1.414 = 2.97V
b) The output idles at VREF = 2.5 V, so it has 2.5 V of room up to 5 V and 2.5 V down to 0 V. In
   the datasheet, find "maximum output voltage" (V_OM) and its conditions (supply and load). It
   tells how close to the rails the output gets: e.g. ±13 V on ±15 V supplies means about 2 V lost
   on each side. The loss depends on the output **current**, not on the load resistance: for each
   row work out I = V_OM / R_L, so you have two points "current → loss".
   Test conditions: V+/V-=±15V
   Swing 1 (RL≥2kΩ): ±13.5V → 1.5 V lost, I = 13.5/2000 = 6.75mA
   Swing 2 (RL≥150Ω): ±11V → 4 V lost, I = 11/150 = 73mA
c) Largest clean peak = 2.5 V − loss. Convert to Vrms. (The loss grows with the peak itself. Fit a
   straight line through the two points, loss = L₀ + k·I: k is a resistance inside the op-amp, so
   peak = (2.5 − L₀) × R_load / (R_load + k), a voltage divider.)
   k = 2.5 V / 66.25 mA = 37.7 Ω, L₀ = 1.5 − 37.7 × 0.00675 = 1.25 V
   peak = 1.25 × 36.7 / (36.7 + 37.7) = 0.61 V, or 0.433 Vrms
d) In dBFS: 20·log₁₀(V_clip_rms / 2.1). Step A4 will show how close your estimate is. -13.7

**Q7 — Datasheet lookups (NJM4556A).**
a) Supply current, both op-amps, no load: typical and maximum, and the conditions. typ=9mA, max = 12mA, V+/V-=±15V,Ta=25˚C
b) Operating supply voltage range: is a single 5 V (= ±2.5 V) inside it? Yes, it is inside:±2V~±18V
c) Input bias current, typical (used in Q2 d). Typ=50nA, max = 500nA
d) Is it stable as a follower (gain 1)? Look for "unity gain stable", a follower example circuit,
   or the gain/phase graph. If unclear, say so; we'll decide together.


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
