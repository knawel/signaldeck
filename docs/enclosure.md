# Enclosure and Front-Panel Concept

## Status

**Concept / not dimensionally finalized**

The mechanical concept is now defined. Final dimensions should be
selected after the VU meters, controls, sockets, and main electronic
modules are chosen and measured.

------------------------------------------------------------------------

# 1. Design Intent

The device should look like a small piece of **laboratory / aircraft
instrumentation**, rather than a conventional modern consumer DAC.

The enclosure combines:

-   two separate **brass front panels** with visible hex screws;
-   two physical analog L/R VU meters;
-   a large physical volume knob;
-   a mechanical toggle power switch;
-   restrained status indicators;
-   a **3D-printed internal structural frame**;
-   natural red/burgundy leather covering the top and sides;
-   compact desktop proportions.

The design should remain functional rather than becoming purely
decorative.

------------------------------------------------------------------------

# 2. Mechanical Architecture

The enclosure is based on three main mechanical elements:

1.  **3D-printed internal frame** --- structural support for the
    electronics and front panels.
2.  **Two removable brass front panels** --- the visible
    instrument/control surface.
3.  **Natural leather outer covering** --- covering the top and sides
    while leaving its natural edges visible where appropriate.

The internal frame should **not have a solid front wall**. Its front
should be an open perimeter structure with a central vertical support
between the two brass panels.

Approximate front structure:

``` text
┌──────────────────────────────────────────────┐
│  FRAME                                       │
│  ┌──────────────────┐│┌───────────────────┐ │
│  │   LEFT BRASS     │││   RIGHT BRASS     │ │
│  │      PANEL       │││       PANEL       │ │
│  │                  │││                   │ │
│  └──────────────────┘│└───────────────────┘ │
│                      │                       │
└──────────────────────────────────────────────┘
                       ↑
              central frame member
```

Each brass panel is independently removable for servicing.

------------------------------------------------------------------------

# 3. Left Instrument Panel

Purpose: **status and measurement**

Planned components:

-   analog LEFT VU meter;
-   analog RIGHT VU meter;
-   USB status indicator;
-   AUDIO/power status indicator;
-   mechanical power toggle, if the final layout places it on this
    panel.

The two meters should be identical and symmetrically positioned.

Likely meter size:

-   approximately 35--45 mm;
-   final size TBD.

Warm/amber backlighting is preferred.

The VU meters, LEDs, and switch should mount **directly to the brass
panel**, not to the structural frame. The panel and its attached
components should therefore be removable as a serviceable assembly.

------------------------------------------------------------------------

# 4. Right Control Panel

Purpose: **user interaction**

Planned components:

-   large volume knob;
-   3.5 mm stereo headphone output;
-   6.35 mm stereo headphone output.

The volume control should be visually dominant.

Preferred knob:

-   aluminium;
-   approximately 30--40 mm diameter;
-   knurled or machined surface;
-   clear position marker.

The potentiometer/encoder and both headphone sockets should mount
**directly to the brass panel from behind**.

------------------------------------------------------------------------

# 5. Front Panels

The front consists of **two separate flat brass panels**, inspired by
removable aircraft instrument panels.

Preferred material:

-   brass sheet, approximately **2 mm thick**;
-   CuZn30/C260 or CuZn37/CW508L, depending on supplier availability;
-   brushed/satin surface.

Preferred manufacturing method:

-   laser or waterjet cutting for the outer contour and through-holes;
-   CNC machining only where required for features that cannot be
    produced economically by 2D cutting;
-   optional laser or CNC engraving for labels.

Each panel should preferably use **four M3 visible hex/socket-head
screws**.

The screws pass through the brass panels into **M3 heat-set threaded
inserts** installed in the printed frame.

``` text
M3 screw
   ↓
brass panel
   ↓
heat-set insert
   ↓
3D-printed frame
```

Visible fasteners are an intentional part of the design.

------------------------------------------------------------------------

# 6. Internal Frame

The structural chassis should be **3D printed for V1** rather than
machined from aluminium.

Preferred materials:

-   PETG for an easy first version;
-   ASA as an alternative where improved temperature resistance and
    finish are useful.

The frame should use ribs and structural members rather than thick solid
walls.

It should provide:

-   perimeter support for the enclosure;
-   a central front member between the two brass panels;
-   heat-set insert bosses for the front-panel screws;
-   PCB mounting points/standoffs;
-   cable-routing features;
-   rear-panel support;
-   mounting points for rubber feet;
-   support for the leather-covered outer surfaces.

The open front should provide access to the controls and instruments
whenever either brass panel is removed.

The design should allow the frame to be reprinted inexpensively if
component placement changes during prototyping.

------------------------------------------------------------------------

# 7. Component Mounting and Serviceability

Front controls and instruments should generally mount to the **brass
panels**:

  Component                      Mechanical mounting
  ------------------------------ -----------------------------------------------------
  VU meters                      Left brass panel
  Status LEDs                    Left brass panel or small PCB immediately behind it
  Power toggle                   Brass panel
  Volume potentiometer/encoder   Right brass panel
  3.5 mm headphone jack          Right brass panel
  6.35 mm headphone jack         Right brass panel
  RP2040/control PCB             Internal printed frame
  DAC PCB                        Internal printed frame
  Amplifier PCB                  Internal printed frame
  VU driver PCB                  Internal printed frame
  USB-C connector                Rear panel/frame

Front-panel wiring should connect to the internal electronics through
**removable connectors** rather than permanent point-to-point soldering
wherever practical.

Removing a brass panel should expose its controls and provide direct
access to the electronics behind it.

The enclosure should be serviceable without removing or damaging the
leather.

------------------------------------------------------------------------

# 8. Leather

The available **natural red/burgundy leather** should cover primarily
the top and sides of the enclosure.

The leather is decorative/protective rather than structural.

Because it is natural leather, the design does **not need to hide every
leather edge**. Cleanly cut exposed edges are acceptable and can
reinforce the handmade character.

The front brass panels remain completely exposed.

The intended contrast is:

**brushed brass + dark structural frame + natural leather**

The leather should not cover screws or other parts that must be removed
for servicing.

------------------------------------------------------------------------

# 9. Rear Panel

The rear panel should remain intentionally minimal.

Current requirement:

``` text
┌─────────────────────────────────┐
│                                 │
│       USB AUDIO + POWER         │
│              USB-C              │
│                ▬                │
│                                 │
└─────────────────────────────────┘
```

One USB-C connection should provide:

-   USB audio data;
-   device power.

No separate DC input is planned for V1.

The rear panel may be integrated into the printed frame or implemented
as a small removable printed panel.

------------------------------------------------------------------------

# 10. Labels and Finish

Labels should be simple and functional, using
technical/instrumentation-style typography.

Examples:

``` text
LEFT      RIGHT

USB       AUDIO

POWER

VOLUME

PHONES 3.5
PHONES 6.35
```

Preferred final production methods:

-   laser engraving;
-   shallow CNC engraving.

Prototype labels may use a simpler temporary method if needed.

The brass may be left to develop a natural patina, or protected later
with wax/clear coating after the desired surface finish is established.

------------------------------------------------------------------------

# 11. Internal Construction

Expected modules:

``` text
┌──────────────────────────────────────┐
│                                      │
│ RP2040          DAC                  │
│   │              │                   │
│   └──── I²S ─────┘                   │
│                                      │
│          amplifier board             │
│                                      │
│          VU driver board             │
│                                      │
│     removable front-panel wiring     │
│                                      │
└──────────────────────────────────────┘
```

Boards should mount to printed standoffs or replaceable mounting plates
inside the frame.

If electromagnetic interference becomes a problem during testing,
provision can be added for a thin grounded aluminium/copper shield or
internal metal plate. The brass front panels can also be bonded to
chassis ground if appropriate.

A full aluminium structural enclosure is **not required for V1**.

------------------------------------------------------------------------

# 12. Mechanical Measurements Required

Before final CAD dimensions are selected, measure:

-   VU meter body depth;
-   VU meter panel cutout dimensions;
-   VU meter mounting method;
-   volume potentiometer/encoder shaft and body dimensions;
-   3.5 mm socket mounting dimensions;
-   6.35 mm socket mounting dimensions and depth;
-   toggle-switch mounting dimensions and depth;
-   LED/indicator mounting dimensions;
-   RP2040/control board dimensions;
-   DAC module dimensions;
-   amplifier PCB dimensions;
-   VU driver PCB dimensions;
-   connector clearance;
-   cable bend radius.

The **VU meters and 6.35 mm socket are likely to determine the minimum
enclosure depth**.

------------------------------------------------------------------------

# 13. Preliminary Size Target

Current visual target:

-   approximately **150--190 mm wide**;
-   approximately **90--130 mm deep**;
-   approximately **55--75 mm high**.

These dimensions remain provisional.

The device should feel substantial on a desk without occupying the
footprint of full-size hi-fi equipment.

------------------------------------------------------------------------

# 14. Feet and Desktop Use

The finished device should have four rubber feet.

Desirable properties:

-   sufficient weight/friction that plugging in 6.35 mm headphones does
    not move the device;
-   controls comfortably accessible while seated;
-   optional slight front-panel inclination if it improves usability.

If the printed-frame construction makes the finished device too light, a
simple internal steel weight plate can be added.

------------------------------------------------------------------------

# 15. Design Principles

When making future enclosure decisions, prioritize:

1.  **Usability**
2.  **Serviceability**
3.  **Mechanical robustness**
4.  **Ease of prototyping and fabrication**
5.  **Compact dimensions**
6.  **Clear visual hierarchy**
7.  **Aircraft/laboratory-instrument character**
8.  **Effective use of brass and natural leather**

Avoid decorative switches, gauges, LEDs, or connectors that do not
perform a real function.

------------------------------------------------------------------------

# 16. Current Open Questions

-   Exact VU meter model and dimensions
-   Final enclosure width/depth/height
-   Flat versus slightly angled front
-   Exact leather coverage and edge treatment
-   Meter illumination implementation
-   Exact volume knob dimensions
-   Final power-switch position
-   Whether both headphone sockets can operate simultaneously
-   Exact engraving/label method
-   Final brass surface protection: natural patina, wax, or clear coat

These should remain open until the electronics prototype and major
mechanical components are available.
