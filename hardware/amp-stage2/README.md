# M4 — Headphone amplifier, stage 2 (OPA1622 + LM27762), hw P2

**Status:** Phase 1 started (2026-11-19) · **Hardware:** P1 → P2 · **Record results in:** `docs/build-log.md`, Milestone 4

Goal: a small, debuggable amp PCB, based on TI reference design TIDA-01341. The LM27762 makes ±4.5 V
from the switched 5 V rail, and the OPA1622 drives the headphones, ground-referenced, with no output
capacitors and no relay. It plugs into the same 8-pin module connector as stage 1 (`docs/pinout.md`).

**Why in parallel with M2.** A board order takes about 4–6 weeks from schematic to a board on the
desk, and a respin adds another 2–3 (`docs/analog-analysis.md`). Reading, schematic and review need no
parts, so they can run while the M2 parts are in the post and the breadboard is being built.

**What can be fixed now, and what must wait:**

| Can be decided now (phases 1–3) | Waits for |
|---|---|
| Rails, LM27762 resistor values, capacitor values and types | — |
| OPA1622 gain, enable/shutdown wiring, pull-ups/pull-downs | — |
| Debug features: jumpers, test points, optional footprints | — |
| Module connector **pinout** (signals) | M2 build confirms it (provisional today) |
| Board outline, mounting holes, connector **position** | M3 tray fit (phase 4) |
| Ordering | schematic review + stock check (phase 5) |

Background and numbers: `docs/analog-analysis.md`; design choices: `docs/architecture.md` section 6.

---

## 0. Before you start

**Download into `hardware/datasheets/`** (and add a row for each to its README: exact revision!):

- OPA1622 datasheet (SBOS727, latest revision), [ti.com/product/OPA1622](https://www.ti.com/product/OPA1622)
- LM27762 datasheet, [ti.com/product/LM27762](https://www.ti.com/product/LM27762)
- TIDA-01341 design guide (TIDUCR0) **and** its design files (schematic, BOM, layout),
  [ti.com/tool/TIDA-01341](https://www.ti.com/tool/TIDA-01341)

**Install:** the current stable KiCad. Nothing to draw yet; phase 2 says when.

The reference design is a starting point, not a copy-paste: TI's board is a power-supply demo, ours
adds the debug features from `docs/build-log.md` M4 and our connector.

---

## 1. The phases

| Phase | What | Output | Gate before the next phase |
|---|---|---|---|
| 1 | Read the three documents, answer Q1–Q10 | answers in section 3 | answers checked |
| 2 | KiCad project, symbols + footprints for both chips | `kicad/` project | footprints checked against the datasheets |
| 3 | Schematic, block by block (section 4) | schematic PDF | ERC clean + review |
| 4 | Layout (needs M3 outline) | gerbers + 3D view | DRC clean + review against TIDA layout rules |
| 5 | Stock check, order with assembly | order | — |
| 6 | Bring-up and tests (section 5) | build-log entries | — |

Phases 1–3 run alongside M2. Phase 4 starts once the M3 tray is measured.

---

## 2. The circuit at a glance

```text
 module connector                     amp board (P2)
 ────────────────                     ──────────────────────────────────────────────────────────
 pin 1  +5 V switched ── J_I5 ──┬── ferrite ──┬── C_bulk ~100 µF
                    (current    |             |
                     jumper)    |          LM27762 ── OUT+  ── J_I+ ──┬── +4.5 V ──► OPA1622 V+
                                |             |   └── OUT−  ── J_I− ──┼── −4.5 V ──► OPA1622 V−
 pin 6  AMP_EN ─────────────────┼────────► EN+ / EN−                  |
 pin 8  AMP_PG ◄────────────────┼───────── PGOOD (open drain?)        |
                                |                                      |
 pin 3  L in ── (C_in opt.) ────┼──────────────────────────────► OPA1622 A ── R_s ──► jack TIP
 pin 4  R in ── (C_in opt.) ────┼──────────────────────────────► OPA1622 B ── R_s ──► jack RING
 pin 5  audio GND ──────────────┴── board ground ─────────────► OPA1622 GND pin, jack SLEEVE
 pin 7  AMP_MUTE ──────────────────────────────────────────────► OPA1622 EN
 pin 2  GND
```

J_I5, J_I+, J_I− are current-measurement jumpers (remove, put the meter across). Every net named
here gets a test point. Values with "?" are what phase 1 settles.

**What is different from stage 1, and why it matters:**

- **Two rails, ±4.5 V.** The output can swing both ways around 0 V, so there is no VREF, no output
  capacitor, no R_bleed and no 2.5 V to charge at power-up. Bass goes down to DC.
- **The price:** a DC error at the output now goes straight into the headphone. So the DC offset
  check (section 5) is the most important measurement on this board.
- **Mute without a relay.** The OPA1622 has an enable pin designed to switch on and off without a
  click; the LM27762 has enables and a power-good output. Firmware sequences them.
- **A switching supply next to audio.** The charge pump switches at about 2 MHz. Far above hearing,
  but it can leak into the audio through bad layout. That's why the TIDA layout rules matter.

---

## 3. Phase 1 — questions

Same rules as M2: answer in your own words with the datasheet page/table you used, post them, and
I'll check. Hints point to where to look, not to the answer. Write units on every number.

**Q1 — OPA1622 pinout.**
a) List the 10 pins of the package we use, with their names and one-line functions.
b) What is the exposed thermal pad, and what must it connect to (or not connect to)?
c) There is a pin called GND in addition to V+ and V−. What is it for?
*Hint: "Pin Configuration and Functions" near the start of the datasheet.*

**Q2 — OPA1622 enable/shutdown (closes the open item in `analog-analysis.md`).**
a) What voltage on EN means "enabled", and what means "shutdown"? Relative to which pin?
b) Can a 3.3 V RP2040 GPIO drive EN directly, or do we need a level shifter? Justify with numbers.
c) What happens if EN is left floating (for example, firmware not running yet)? So: pull-up or
pull-down on the board, and which state do we *want* at power-up?
d) What does the output look like in shutdown: high impedance, or tied to something?
*Hint: Electrical Characteristics table, "Enable" rows, and the section on the shutdown function.*

**Q3 — How loud can it go at ±4.5 V?**
a) Find the output voltage swing spec (how close to each rail the output gets) at a load near 32 Ω.
b) With ±4.5 V rails, what is the largest sine **peak**, then **RMS**, you can get into 32 Ω?
c) The PCM5102A full scale is about 2.1 Vrms. At gain 1, does it fit? With how much margin?
d) Same into 16 Ω, then check the current: is the OPA1622 output current limit or the LM27762's
250 mA per rail the tighter limit? (Two channels share the rails.)
*Hint: worked example in M2 section 3.1, idea 1. Peak = RMS × 1.414.*

**Q4 — LM27762 output voltages.**
a) Input voltage range? Does a USB 5 V that sags to 4.75 V under load stay inside it?
b) How is each output voltage set? Write the formula for OUT+ and for OUT−.
c) Choose standard (E96) resistor values for +4.5 V and −4.5 V. Also for ±4.0 V and ±5.0 V
(the build log wants all three as footprint options).
d) The positive LDO needs some headroom (dropout) above its output. From 4.75 V in, is +4.5 V
realistic? If not, what is the highest safe setting?
*Hint: "Setting the Output Voltage" in the Application section; dropout in the Electrical Characteristics.*

**Q5 — LM27762 enables and power-good.**
a) What are EN+ and EN−? Can they be tied together? What does TIDA-01341 do?
b) PGOOD: what is the output type? If open drain, it needs a pull-up: to which voltage, so the
RP2040 (3.3 V input) is safe?
c) Logic thresholds of EN+/EN−: OK for 3.3 V?

**Q6 — Capacitors.**
a) List every capacitor the LM27762 needs (input, flying, charge-pump output, LDO outputs), with
value and recommended dielectric.
b) Ceramic capacitors lose capacitance under DC voltage ("DC bias derating"). Does the datasheet
say anything about it? What case size / voltage rating would you pick so a "4.7 µF" is still a
useful value at 5 V?
c) Why does `analog-analysis.md` say "never large capacitors on the LDO outputs"? Find the datasheet
sentence that limits output capacitance.

**Q7 — Reading TIDUCR0 (the TIDA-01341 guide).**
a) What did TI measure on their board (ripple, noise, THD+N)? Note the numbers and conditions.
b) Write down the layout rules they give (where the flying capacitor goes, ground, loop areas).
c) Compare their schematic with the LM27762 datasheet's typical application. Any differences?

**Q8 — Power budget on the 5 V side.**
a) Charge pump + LDO: roughly what efficiency does the datasheet show at a few tens of mA?
b) Both channels play 1 Vrms into 32 Ω (sine). Estimate the current from the 5 V rail.
Compare with the 500 mA USB limit and what the RP2040 + DAC already use.

**Q9 — Stability and protection.**
a) Is the OPA1622 stable at gain 1 (as a voltage follower)? Where does it say so?
b) What does the datasheet say about capacitive loads and an output series resistor? (Headphone
cables are capacitive.) What value would you put on the R_s footprint?
c) What protects the chip if the jack is shorted while playing?

**Q10 — Input side.**
a) Is the PCM5102A output centred on 0 V? Look up its DC output offset spec (`pcm5102a.pdf`).
b) With DC coupling and gain 1, that offset appears at the headphone. Is it acceptable? (Compare
with Q2-style datasheet numbers for the OPA1622's own offset, and with a few mV being harmless.)
c) So: input capacitors fitted, or footprint only with a 0 Ω/wire option? What input impedance
does the DAC then see?

### Answers

(Post them in chat; checked answers get recorded here.)

---

## 4. Phase 3 — schematic blocks (after phase 1)

Draw and review one block at a time, in this order:

1. **Connector + input supply:** 8-pin connector, J_I5, ferrite (with 0 Ω bypass footprint), C_bulk.
2. **LM27762:** feedback resistors for ±4.5 V (footprints for the other options), all capacitors
   from Q6, J_I+ and J_I−, EN/PGOOD wiring from Q5, test points on OUT+, OUT−, CPOUT, PGOOD.
3. **OPA1622:** follower wiring, gain footprints (1 to ~3; gain 1 fitted), feedback-capacitor
   footprint, local decoupling, EN pull from Q2c, R_s, optional C_in.
4. **Outputs:** pads or a header to the jacks (the jacks sit on the front panel, not on this board).
5. **Debug extras:** ground test points ("scope ground spots"), revision text "P2" on silkscreen.

The review checklist (`docs/build-log.md` M4, "before ordering") applies at the end.

---

## 5. Phase 6 — bring-up and tests (preview)

Same discipline as M2: rails first, chip with no load, dummy loads, then headphones.

- [ ] LM27762 alone (OPA1622 jumpers open): OUT+ and OUT− within tolerance, PGOOD behaves
- [ ] idle current per rail near the datasheet value, chips cool (no oscillation)
- [ ] **DC offset across the load: a few mV at most**, both channels, enabled and in shutdown
- [ ] output level vs digital level, where clipping starts (compare with Q3)
- [ ] no hum/hiss at maximum with nothing playing (charge-pump leakage would show here)
- [ ] no click on EN on/off, power-up/down, USB plug/unplug
- [ ] 16 Ω accidental load: reduced output, no damage
- [ ] measured 5 V current → USB `bMaxPower`
