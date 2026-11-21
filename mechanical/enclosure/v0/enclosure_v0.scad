// signaldeck — prototype enclosure v0 (M3)
//
// A first, cheap, printable case to try the placement of boards, knob, sockets and meters.
// Not the final design: printed front panels stand in for the brass ones, the meters are
// placeholder cut-outs, and the floor is a grid of holes so boards can move without a reprint.
//
// Coordinates: x = left→right seen from the front, y = front→back, z = up.
// The front frame's front face is at y = 0; the panels sit in front of it (y < 0).
// The rear frame is a mirror image of the front frame: its outer face is at y = D and the
// rear plate sits behind it (y > D). The case also works without the rear plate (open back).
//
// Export one part:  openscad -D 'part="tray"' -o tray.stl enclosure_v0.scad   (see export.sh)
// Parts: "assembly" (preview only), "tray", "panel_left", "panel_right", "panel_rear", "lid",
//        "base" (steel plate, 3D preview), "base_2d" (its outline for laser cutting: export as DXF)

part = "assembly";

// ------------------------------------------------------------------ main dimensions
W        = 190;     // outside width   (concept: 150–190)
D        = 115;     // outside depth   (concept: 90–130)
H        = 70;      // outside height including lid (concept: 55–75)
wall     = 2.4;     // side walls
floor_t  = 2.4;
lid_t    = 2.4;
frame_t  = 4;       // front and rear frames the panels/plate screw onto
Hb       = H - lid_t;   // height of the tray (walls)

// ------------------------------------------------------------------ front panels
panel_t      = 3;       // printed panel; the brass one will be 2 mm
panel_margin = 3;       // frame visible around the panels
panel_gap    = 4;       // gap between the panels: the central frame member shows through
panel_w      = (W - 2*panel_margin - panel_gap) / 2;
panel_h      = Hb - 2*panel_margin;
panel_z0     = panel_margin;
left_x0      = panel_margin;
right_x0     = panel_margin + panel_w + panel_gap;

overlap      = 4;       // panel overlaps the frame window by this much on each side
screw_inset  = 4.5;     // panel screw centre from the panel edges
boss_d       = 9;       // frame bosses behind the panel screws (≥ 2 × insert hole)
boss_depth   = 8;       // how far the bosses stick out behind the frame
notch        = screw_inset + boss_d/2 + 0.5;   // window corners left solid around the bosses

// Fasteners: M3 countersunk (ISO 10642 hex or ISO 14581 Torx): × 8 for panels, × 10 for lid + base,
// into brass heat-set inserts. The screw turns in brass, so it can go in and out many times.
screw_clear  = 3.4;     // M3 clearance hole
cs_d         = 6.5;     // countersink diameter at the surface (90°); M3 heads are ~5.5–6 mm,
                        // so the head ends up slightly below flush
insert_hole  = 4.0;     // M3 heat-set insert hole: MEASURE (check the insert's spec sheet)
insert_len   = 5.7;     // insert length (common M3 × 5.7); the hole is 1 mm deeper for melt
insert_depth = insert_len + 1;

// Printing (FDM, tray upright): the top of each frame window is a ~71 mm bridge. A thin rib in
// the middle of each window halves it; cut the ribs off with flush cutters after printing.
// Set false for SLS/MJF (no supports needed) or if your printer bridges 71 mm cleanly.
breakaway_ribs = true;
rib_w          = 1.2;   // three 0.4 mm lines: strong enough to print, easy to cut

// ------------------------------------------------------------------ front components
// All MEASURE YOUR PARTS: hole sizes depend on the exact jack/encoder/toggle bought.
// Positions are [u, v] in mm from the panel's lower-left corner, seen from the front.
toggle_hole  = 6.4;   toggle_pos  = [12, 31];   // E-Switch 100DP1T1B1M1REH, 1/4"-40 UNS bushing
enc_hole     = 7.2;   enc_pos     = [37, 31];   // Bourns PEC11R-4220F-N0024, M7×0.75 bushing
knob_d       = 30;                              // for the preview and clearance check
jack35_hole  = 6.2;   jack35_pos  = [61, 31];   // Amphenol ACJS-MV35-3S: MEASURE the thread
jack635_hole = 9.5;   jack635_pos = [77, 31];   // Neutrik NMJ6HC-S, datasheet "chassis 11.4 mm": MEASURE

meter_cut    = [38, 30];                        // placeholder meter window
meter_pos    = [[24, 37], [66, 37]];
meter_depth  = 30;                              // body depth behind the panel (preview)
led_hole     = 5.1;                             // 5 mm LED, or ~8 mm for a bezel/holder
led_pos      = [[24, 12], [45, 12], [66, 12]];
led_labels   = ["PWR", "USB", "SIG"];

engrave      = 0.6;     // label depth
font         = "Liberation Sans:style=Bold";

// ------------------------------------------------------------------ inside
grid_pitch   = 10.16;   // 4 × 2.54 mm: perfboard holes 4 apart line up with the grid
grid_hole    = 3.3;     // M3 standoffs / screws, also cable-tie points
grid_margin  = 14;

// ------------------------------------------------------------------ rear plate
// One plate across the whole back, screwed to the rear frame with the same 8-screw pattern as
// the two front panels (so two half plates also fit). Seen from the FRONT, x as everywhere else.
rear_w       = W - 2*panel_margin;
usb_pos      = [30, 21];    // [x, z] centre of the USB-C opening in the rear plate: MEASURE
usb_cut      = [13, 8];     // generous; a USB-C plug overmould is ~12 × 7 mm

// ------------------------------------------------------------------ steel base plate
// A steel plate under the tray adds weight and covers the floor holes. It is set in from the
// edges so it reads as a dark shadow line under the case ("plinth"). The four lid screws go
// through plate + floor into the lid posts, so they hold everything. Rubber feet go on the steel.
base_t       = 3;       // 3 mm mild steel ≈ 0.48 kg (2 mm ≈ 0.32 kg, but too thin for countersinks)
base_inset   = 2;       // shadow gap: plate edge this far in from the tray's outside
base_r       = 3;       // corner radius

// ------------------------------------------------------------------ lid fixing
// The lid has four posts hanging down along the side walls. Each post has an insert at its lower
// end; M3 × 10 countersunk screws come up through the base plate and the floor, so the top stays clean.
lid_post     = [10, 10];    // post cross-section [x, y]
lid_post_y   = [30, 85];    // clear of the frame bosses
post_gap     = 0.3;         // post ends this far above the floor: the screw pulls the lid down tight

// placeholders for the preview (sizes: MEASURE)
carrier      = [60, 40];  carrier_pos = [16, 70];    // perfboard with RP2040-Zero + DAC
amp_board    = [70, 50];  amp_pos     = [104, 50];   // stage 1 amplifier perfboard (5 × 7 cm)
standoff_h   = 8;

$fn = 48;

// ================================================================== helpers

// a hole through a front panel, along y, at panel coordinates [u, v]
module panel_hole(p, d, x0) {
    translate([x0 + p[0], 1, panel_z0 + p[1]]) rotate([90, 0, 0])
        cylinder(d = d, h = panel_t + 2);
}

// engraved text on the panel front, centred at [u, v]
module panel_text(s, p, x0, size = 3.2) {
    translate([x0 + p[0], -panel_t + engrave, panel_z0 + p[1]]) rotate([90, 0, 0])
        linear_extrude(engrave + 0.01)
            text(s, size = size, font = font, halign = "center", valign = "center");
}

// a countersunk M3 hole along z, cone opening at z = 0 (orient it with rotate/translate)
module countersunk_hole(len) {
    translate([0, 0, -0.01]) cylinder(d1 = cs_d, d2 = screw_clear, h = (cs_d - screw_clear) / 2);
    translate([0, 0, -1]) cylinder(d = screw_clear, h = len + 2);
}

module panel_screw_holes(x0) {
    for (u = [screw_inset, panel_w - screw_inset], v = [screw_inset, panel_h - screw_inset])
        translate([x0 + u, -panel_t, panel_z0 + v]) rotate([-90, 0, 0]) countersunk_hole(panel_t);
}

// 2D teardrop pointing toward -y (drawn in (x, z): pointing down), so a horizontal boss has a
// 45° underside and prints without support
module teardrop(d) {
    hull() {
        circle(d = d);
        translate([0, -(d / 2 * sqrt(2) - 0.1)]) square(0.2, center = true);
    }
}

// mirror front-frame geometry onto the rear
module at_rear() {
    translate([0, D, 0]) mirror([0, 1, 0]) children();
}

module rounded_rect(size, r) {
    offset(r) offset(-r) square(size, center = true);
}

// ================================================================== front panels

module panel_blank(x0) {
    translate([x0, -panel_t, panel_z0]) cube([panel_w, panel_t, panel_h]);
}

module panel_right() {
    x0 = right_x0;
    difference() {
        panel_blank(x0);
        panel_screw_holes(x0);
        panel_hole(toggle_pos,  toggle_hole,  x0);
        panel_hole(enc_pos,     enc_hole,     x0);
        panel_hole(jack35_pos,  jack35_hole,  x0);
        panel_hole(jack635_pos, jack635_hole, x0);
        panel_text("POWER",  [toggle_pos[0], 14], x0, 3);
        panel_text("VOLUME", [enc_pos[0], 9], x0);
        panel_text("PHONES", [(jack35_pos[0] + jack635_pos[0]) / 2, 14], x0, 3);
        panel_text("signaldeck", [panel_w / 2, panel_h - 7], x0, 4);
    }
}

module panel_left() {
    x0 = left_x0;
    difference() {
        panel_blank(x0);
        panel_screw_holes(x0);
        for (p = meter_pos)
            translate([x0 + p[0], 1, panel_z0 + p[1]]) rotate([90, 0, 0])
                linear_extrude(panel_t + 2) rounded_rect(meter_cut, 2);
        for (i = [0 : len(led_pos) - 1]) {
            panel_hole(led_pos[i], led_hole, x0);
            panel_text(led_labels[i], [led_pos[i][0], led_pos[i][1] - 7], x0, 2.6);
        }
    }
}

// ================================================================== rear plate

// engraved text on the rear plate, readable from behind; p = [x, z] as seen from the front
module rear_text(s, p, size = 3.2) {
    translate([p[0], D + panel_t - engrave, p[1]]) rotate([0, 0, 180]) rotate([90, 0, 0])
        linear_extrude(engrave + 0.01)
            text(s, size = size, font = font, halign = "center", valign = "center");
}

module panel_rear() {
    difference() {
        translate([panel_margin, D, panel_z0]) cube([rear_w, panel_t, panel_h]);
        for (x0 = [left_x0, right_x0],
             u = [screw_inset, panel_w - screw_inset], v = [screw_inset, panel_h - screw_inset])
            translate([x0 + u, D + panel_t, panel_z0 + v]) rotate([90, 0, 0])
                countersunk_hole(panel_t);
        // USB-C opening
        translate([usb_pos[0], D - 1, usb_pos[1]]) rotate([-90, 0, 0])
            linear_extrude(panel_t + 2) rounded_rect(usb_cut, 2);
        rear_text("USB", [usb_pos[0], usb_pos[1] - 9], 3);
        rear_text("signaldeck v0", [W / 2, panel_h - 7], 4);
    }
}

// ================================================================== tray

// window in the front frame behind one panel, with solid corners for the screw bosses
module frame_window(x0) {
    translate([x0 + overlap, -1, panel_z0 + overlap])
        rotate([-90, 0, 0]) mirror([0, 1, 0])          // draw in (x, z), extrude along +y
            linear_extrude(frame_t + 2)
                difference() {
                    square([panel_w - 2*overlap, panel_h - 2*overlap]);
                    for (cx = [0, panel_w - 2*overlap], cz = [0, panel_h - 2*overlap])
                        translate([cx - (notch - overlap), cz - (notch - overlap)])
                            square(2 * (notch - overlap));
                }
}

// Bosses behind the frame, shaped so the tray prints upright with no supports and no thin gaps:
//  - bottom row: a solid block from the floor up with a round top (vertical sides, no overhang);
//  - top row at the side walls: teardrop merged into the wall (45° underside, no slot at the wall);
//  - top row at the centre bar: plain teardrops.
// Each profile is drawn in (x, z) and extruded backwards along +y.
module boss_profile(cx, cz) {
    r = boss_d / 2;
    if (cz < Hb / 2)                                   // bottom row: down to the floor
        hull() { translate([cx, cz]) circle(r = r); translate([cx - r, 0]) square([boss_d, 0.01]); }
    else if (cx - r < wall + 3)                        // top row, left wall
        hull() { translate([cx, cz]) teardrop(boss_d); translate([r, cz]) teardrop(boss_d); }
    else if (cx + r > W - wall - 3)                    // top row, right wall
        hull() { translate([cx, cz]) teardrop(boss_d); translate([W - r, cz]) teardrop(boss_d); }
    else
        translate([cx, cz]) teardrop(boss_d);
}

module frame_bosses(x0) {
    for (u = [screw_inset, panel_w - screw_inset], v = [screw_inset, panel_h - screw_inset])
        translate([0, frame_t - 0.01, 0]) rotate([90, 0, 0]) mirror([0, 0, 1])
            linear_extrude(boss_depth) boss_profile(x0 + u, panel_z0 + v);
}

// insert holes, pressed in from the front face of the frame
module frame_insert_holes(x0) {
    for (u = [screw_inset, panel_w - screw_inset], v = [screw_inset, panel_h - screw_inset])
        translate([x0 + u, -1, panel_z0 + v]) rotate([-90, 0, 0])
            cylinder(d = insert_hole, h = insert_depth + 1);
}

function lid_post_points() = [
    for (y = lid_post_y, x = [wall + 0.3 + lid_post[0]/2, W - wall - 0.3 - lid_post[0]/2]) [x, y]
];

// the frame plate with its bosses, and what gets cut out of it (used front and rear)
module frame_solid() {
    cube([W, frame_t, Hb]);
    frame_bosses(left_x0);
    frame_bosses(right_x0);
}

module frame_cuts() {
    frame_window(left_x0);
    frame_window(right_x0);
    frame_insert_holes(left_x0);
    frame_insert_holes(right_x0);
}

module breakaway_rib(x0) {
    translate([x0 + panel_w/2 - rib_w/2, 0, panel_z0 + overlap - 0.01])
        cube([rib_w, frame_t, panel_h - 2*overlap + 0.02]);
}

module tray() {
    if (breakaway_ribs)
        for (x0 = [left_x0, right_x0]) { breakaway_rib(x0); at_rear() breakaway_rib(x0); }
    difference() {
        union() {
            // floor, side walls, front frame, rear frame
            cube([W, D, floor_t]);
            cube([wall, D, Hb]);
            translate([W - wall, 0, 0]) cube([wall, D, Hb]);
            frame_solid();
            at_rear() frame_solid();
        }
        frame_cuts();
        at_rear() frame_cuts();
        // lid screws come up through the base plate and the floor (countersunk in the plate)
        for (p = lid_post_points())
            translate([p[0], p[1], -1]) cylinder(d = screw_clear, h = floor_t + 2);
        // mounting grid in the floor, countersunk underneath so standoff screws sit flush
        // (M3 × 6 countersunk) and the base plate lies flat
        for (x = [grid_margin : grid_pitch : W - grid_margin],
             y = [grid_margin + frame_t : grid_pitch : D - grid_margin])
            translate([x, y, 0]) countersunk_hole(floor_t);
    }
}

// ================================================================== lid

module lid() {
    z0 = floor_t + post_gap;                    // lower end of the posts
    difference() {
        union() {
            translate([0, -panel_t, Hb]) cube([W, D + 2*panel_t, lid_t]);
            for (p = lid_post_points())
                translate([p[0] - lid_post[0]/2, p[1] - lid_post[1]/2, z0])
                    cube([lid_post[0], lid_post[1], Hb - z0 + 0.01]);
        }
        for (p = lid_post_points())
            translate([p[0], p[1], z0 - 0.01]) cylinder(d = insert_hole, h = insert_depth);
    }
}

// ================================================================== steel base plate

module base_2d() {
    difference() {
        translate([base_inset, base_inset])
            offset(r = base_r) offset(delta = -base_r) square([W - 2*base_inset, D - 2*base_inset]);
        for (p = lid_post_points()) translate(p) circle(d = screw_clear);
    }
}

module base() {
    difference() {
        translate([0, 0, -base_t]) linear_extrude(base_t) base_2d();
        for (p = lid_post_points())                  // 90° countersink on the underside
            translate([p[0], p[1], -base_t]) countersunk_hole(base_t);
    }
}

// ================================================================== preview placeholders

module front_component(x0, p, d, depth, col) {
    color(col) translate([x0 + p[0], frame_t, panel_z0 + p[1]]) rotate([-90, 0, 0])
        cylinder(d = d, h = depth);
}

module placeholders() {
    // right panel: knob in front, bodies behind
    color("silver") translate([right_x0 + enc_pos[0], -panel_t, panel_z0 + enc_pos[1]])
        rotate([90, 0, 0]) cylinder(d = knob_d, h = 18);
    color("dimgray") translate([right_x0 + toggle_pos[0], -panel_t, panel_z0 + toggle_pos[1]])
        rotate([90 - 15, 0, 0]) cylinder(d = 3, h = 12);
    color("dimgray") translate([right_x0 + enc_pos[0] - 6.5, 0, panel_z0 + enc_pos[1] - 6.5])
        cube([13, 20, 13]);
    color("dimgray") translate([right_x0 + toggle_pos[0] - 6.5, 0, panel_z0 + toggle_pos[1] - 4])
        cube([13, 18, 8]);
    front_component(right_x0, jack35_pos, 11, 16, "dimgray");
    front_component(right_x0, jack635_pos, 19, 34, "dimgray");
    // left panel: meters (amber faces) and LEDs
    for (p = meter_pos) {
        color("orange") translate([left_x0 + p[0] - meter_cut[0]/2, -panel_t + 0.5, panel_z0 + p[1] - meter_cut[1]/2])
            cube([meter_cut[0], 1, meter_cut[1]]);
        color("dimgray") translate([left_x0 + p[0] - meter_cut[0]/2, 0, panel_z0 + p[1] - meter_cut[1]/2])
            cube([meter_cut[0], meter_depth, meter_cut[1]]);
    }
    for (i = [0 : len(led_pos) - 1])
        color(["orange", "deepskyblue", "lime"][i])
            translate([left_x0 + led_pos[i][0], -panel_t - 1, panel_z0 + led_pos[i][1]])
                sphere(d = 5);
    // boards on standoffs
    color("darkgreen") translate([carrier_pos[0], carrier_pos[1], floor_t + standoff_h])
        cube([carrier[0], carrier[1], 1.6]);
    // RP2040-Zero (18 × 23.5) on headers, USB-C edge right behind the rear plate
    color("black") translate([usb_pos[0] - 9, D - 23.5 - 0.5, usb_pos[1] - 2.6])
        cube([18, 23.5, 1.2]);
    color("silver") translate([usb_pos[0] - 4.5, D - 7.5 - 0.5, usb_pos[1] - 1.6])
        cube([9, 7, 3.2]);
    // PCM5102A module (~32 × 24: MEASURE)
    color("purple") translate([carrier_pos[0] + 30, carrier_pos[1] + 4, floor_t + standoff_h + 10])
        cube([24, 32, 1.6]);
    color("darkgreen") translate([amp_pos[0], amp_pos[1], floor_t + standoff_h])
        cube([amp_board[0], amp_board[1], 1.6]);
}

// ================================================================== output

module assembly(show_lid = true, show_rear = true) {
    color("dimgray", 1) tray();
    color("goldenrod") panel_left();
    color("goldenrod") panel_right();
    if (show_rear) color("goldenrod") panel_rear();
    color("#2b2b2b") base();
    placeholders();
    if (show_lid) color("saddlebrown", 0.35) lid();
}

// print orientation for each part
if (part == "assembly")         assembly(true);
else if (part == "assembly_open") assembly(false);
else if (part == "tray")        tray();
else if (part == "panel_left")  panel_print() panel_left();
else if (part == "panel_right") panel_print() panel_right();
else if (part == "panel_rear")  translate([-panel_margin, -panel_z0, D + panel_t]) rotate([-90, 0, 0]) panel_rear();
else if (part == "back_open")   assembly(false, false);
else if (part == "base")        translate([0, 0, base_t]) base();
else if (part == "base_2d")     base_2d();
else if (part == "lid")         translate([0, D + panel_t, 0]) rotate([180, 0, 0]) translate([0, 0, -Hb - lid_t]) lid();

// panels print face down (labels on the bed side come out crisp); moved to the origin
module panel_print() {
    translate([0, panel_h, panel_t]) rotate([90, 0, 0])
        translate([-(part == "panel_left" ? left_x0 : right_x0), 0, -panel_z0]) children();
}
