#!/usr/bin/env python3
"""Draw the M2 amp stage 1 schematic (standard symbols) and the NJM4556AD wiring view.

Writes hardware/amp-stage1/schematic.svg and hardware/amp-stage1/wiring.svg.

Needs schemdraw:  pip install schemdraw==0.19
Usage:            python3 hardware/amp-stage1/tools/draw_schematic.py

Values you work out in README section 3.2 (Q1-Q5) are left as names, not numbers.
"""
from pathlib import Path

import schemdraw
import schemdraw.elements as elm

HERE = Path(__file__).resolve().parent.parent
OUT = HERE / "schematic.svg"
OUT_WIRING = HERE / "wiring.svg"

schemdraw.use("svg")
schemdraw.config(fontsize=11, unit=2.5)


def title(d, xy, text):
    d.add(elm.Label().at(xy).label(text, loc="center", fontsize=13, halign="left"))


with schemdraw.Drawing(show=False) as d:
    # ------------------------------------------------------------------
    # 1. Supply filter and VREF
    # ------------------------------------------------------------------
    title(d, (0, 3.2), "1. Supply filter and mid-supply reference")
    d += elm.Vdd().at((0, 1.5)).label("USB 5 V\n(Part B: via soft-start)")
    d += elm.Line().down(1.5)
    d += elm.Inductor2(loops=2).right().label("FB1\nferrite")
    d += elm.Dot()
    vplus = d.here
    d.add(elm.Label().at((vplus[0] + 0.2, vplus[1] + 0.4)).label("V+ (filtered)", halign="left"))
    d += elm.Capacitor(polar=True).at(vplus).down().label("C_bulk\n470 µF", loc="bottom")
    d += elm.Ground()

    d += elm.Line().at(vplus).right(2.5)
    d += elm.Dot()
    n2 = d.here
    d += elm.Capacitor().down().label("100 nF", loc="bottom")
    d += elm.Ground()
    d += elm.Line().at(n2).right(1.8)
    d += elm.Dot()
    n3 = d.here
    d += elm.Capacitor(polar=True).down().label("10 µF", loc="bottom")
    d += elm.Ground()
    d += elm.Label().at((n2[0] + 0.9, n2[1] - 3.4)).label(
        "close to U1 pins 8 / 4\n(see wiring view)", fontsize=9)

    d += elm.Line().at(n3).right(2.5)
    d += elm.Resistor().down().label("R_top", loc="bottom")
    d += elm.Dot()
    vref = d.here
    d += elm.Resistor().down().label("R_bot", loc="bottom")
    d += elm.Ground()
    d += elm.Line().at(vref).right(2.5)
    d += elm.Dot()
    nref = d.here
    d += elm.Capacitor(polar=True).down().label("C_ref\n47 µF", loc="bottom")
    d += elm.Ground()
    d += elm.Line().at(nref).right(1.5)
    d += elm.Dot(open=True).label("VREF (= V+/2)", loc="right")

    # ------------------------------------------------------------------
    # 2. One channel (L shown; R is identical on U1B, pins 5/6/7)
    # ------------------------------------------------------------------
    y = -11
    title(d, (0, y + 3.2), "2. One channel  (L on U1A shown; R is identical on U1B: pins 5 +, 6 -, 7 out)")
    d += elm.Dot(open=True).at((0, y)).label("DAC L OUT", loc="left")
    d += elm.Line().at((0, y)).right(0.5)
    d += elm.Capacitor().right().label("C_in\n(film)")
    d += elm.Dot()
    nin = d.here
    d += elm.Resistor().down().label("R_b", loc="bottom")
    d += elm.Dot(open=True).label("VREF", loc="bottom")
    d += elm.Line().at(nin).right(1)
    op = d.add(elm.Opamp(sign=True, leads=True).anchor("in2").label("U1A\nNJM4556A", loc="center", ofst=(-0.3, 0), fontsize=9))
    d += elm.Label().at((op.in2[0] + 0.15, op.in2[1] - 0.35)).label("3", fontsize=8)
    d += elm.Label().at((op.in1[0] + 0.15, op.in1[1] + 0.35)).label("2", fontsize=8)
    d += elm.Label().at((op.out[0] - 0.1, op.out[1] + 0.35)).label("1", fontsize=8)
    # follower feedback: out -> (−)
    d += elm.Line().at(op.out).right(0.5)
    d += elm.Dot()
    nout = d.here
    d += elm.Line().up(2)
    fb_top = d.here
    d += elm.Line().left().tox(op.in1[0])
    d += elm.Line().down().toy(op.in1[1])

    d += elm.Resistor().at(nout).right().label("R_s\n4.7 Ω")
    d += elm.Capacitor(polar=True).right().label("C_out\n1000 µF")
    d += elm.Dot()
    nb = d.here
    d += elm.Resistor().down().label("R_bleed", loc="bottom")
    d += elm.Ground()
    d += elm.Line().at(nb).right(0.5)
    d += elm.Switch().right().label("K1 contact\n(mute)")
    d += elm.Line().right(0.5)
    d += elm.Dot(open=True).label("jack TIP (L)", loc="right")
    d += elm.Dot(open=True).at((d.here[0], d.here[1] - 2.5)).label("jack SLEEVE", loc="right")
    sleeve = d.here
    d += elm.Line().at(sleeve).left(0.8)
    d += elm.Ground()

    # ------------------------------------------------------------------
    # 3. Mute relay driver
    # ------------------------------------------------------------------
    y = -23
    title(d, (0, y + 6), "3. Mute relay driver")
    d += elm.Dot(open=True).at((0, y)).label("GPIO (3.3 V)", loc="left")
    d += elm.Line().at((0, y)).right(0.5)
    d += elm.Resistor().right().label("R_base")
    q = d.add(elm.BjtNpn(circle=True).anchor("base").label("Q1\nBC547", loc="right"))
    d += elm.Ground().at(q.emitter)
    d += elm.Line().at(q.collector).up(0.5)
    d += elm.Dot()
    nc = d.here
    d += elm.Inductor2(loops=3).up().label("K1\ncoil", loc="top")
    d += elm.Dot()
    ntop = d.here
    d += elm.Vdd().label("5 V")
    d += elm.Line().at(nc).right(2)
    d += elm.Diode().up().label("D1\n1N4148", loc="bottom")   # cathode toward 5 V
    d += elm.Line().left().tox(ntop[0])

d.save(str(OUT))
print(f"wrote {OUT}")


# ----------------------------------------------------------------------
# Wiring view: U1 = NJM4556AD, DIP-8 seen from the top (notch up)
# ----------------------------------------------------------------------
def pin_net(d, at, text, direction):
    d += getattr(elm.Line().at(at), direction)(3.2)
    end = d.here
    if direction == "left":
        d.add(elm.Label().at((end[0] - 0.15, end[1])).label(text, halign="right", valign="center"))
    else:
        d.add(elm.Label().at((end[0] + 0.15, end[1])).label(text, halign="left", valign="center"))


with schemdraw.Drawing(show=False) as w:
    u = w.add(elm.Ic(
        size=(3.4, 6),
        pins=[
            # schemdraw counts L/R slots from the bottom
            elm.IcPin(name="OUT A", pin="1", side="L", slot="4/4", anchorname="p1"),
            elm.IcPin(name="-IN A", pin="2", side="L", slot="3/4", anchorname="p2"),
            elm.IcPin(name="+IN A", pin="3", side="L", slot="2/4", anchorname="p3"),
            elm.IcPin(name="V-", pin="4", side="L", slot="1/4", anchorname="p4"),
            elm.IcPin(name="+IN B", pin="5", side="R", slot="1/4", anchorname="p5"),
            elm.IcPin(name="-IN B", pin="6", side="R", slot="2/4", anchorname="p6"),
            elm.IcPin(name="OUT B", pin="7", side="R", slot="3/4", anchorname="p7"),
            elm.IcPin(name="V+", pin="8", side="R", slot="4/4", anchorname="p8"),
        ],
        leadlen=1.4, lofst=0.15,
    ).theta(0).at((0, 0)).label("U1  NJM4556AD\n(notch up, pin 1 top left)", loc="top", fontsize=10))
    title(w, (u.p1[0] - 9, u.p1[1] + 2.2), "Wiring view: U1 pins, top view")

    # follower loops: 1 -> 2 and 7 -> 6, short jumpers on the pin leads
    w += elm.Dot().at((u.p1[0] + 0.6, u.p1[1]))
    w += elm.Line().down().toy(u.p2[1])
    w += elm.Dot()
    w += elm.Dot().at((u.p7[0] - 0.6, u.p7[1]))
    w += elm.Line().down().toy(u.p6[1])
    w += elm.Dot()

    pin_net(w, u.p1, "R_s L -> C_out L -> relay -> jack TIP", "left")
    pin_net(w, u.p2, "jumper to pin 1 (follower)", "left")
    pin_net(w, u.p3, "C_in L (from DAC L OUT) and R_b L (to VREF)", "left")
    pin_net(w, u.p4, "GND", "left")
    pin_net(w, u.p8, "V+ (filtered)", "right")
    pin_net(w, u.p7, "R_s R -> C_out R -> relay -> jack RING", "right")
    pin_net(w, u.p6, "jumper to pin 7 (follower)", "right")
    pin_net(w, u.p5, "C_in R (from DAC R OUT) and R_b R (to VREF)", "right")

    # decoupling: drawn separately below, between the two supply pins
    x0, y0 = u.p4[0] + 2.2, u.p4[1] - 2.2
    w += elm.Dot(open=True).at((x0, y0)).label("pin 8 (V+)", loc="left")
    w += elm.Line().right(1.5)
    w += elm.Dot()
    a = w.here
    w += elm.Capacitor().down().label("100 nF", loc="bottom")
    w += elm.Dot()
    b = w.here
    w += elm.Line().at(a).right(2)
    w += elm.Capacitor(polar=True).down().label("10 µF", loc="bottom")
    w += elm.Line().left().tox(b[0])
    w += elm.Line().at(b).left().tox(x0)
    w += elm.Dot(open=True).label("pin 4 (GND)", loc="left")
    w.add(elm.Label().at((x0 + 5, y0 - 1.25)).label(
        "right at the chip,\nleads as short as possible", halign="left", fontsize=10))

w.save(str(OUT_WIRING))
print(f"wrote {OUT_WIRING}")
