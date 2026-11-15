#!/bin/sh
# Export the printable parts (in print orientation) and preview images.
# Needs OpenSCAD on the PATH (macOS: brew install --cask openscad@snapshot).
set -e
cd "$(dirname "$0")"
mkdir -p stl dxf images
for p in tray panel_left panel_right panel_rear lid; do
    echo "exporting $p"
    openscad -q -D "part=\"$p\"" -o "stl/$p.stl" enclosure_v0.scad
done
echo "exporting base plate (DXF for laser cutting, STL for a printed mock-up)"
openscad -q -D 'part="base_2d"' -o dxf/base_plate.dxf enclosure_v0.scad
openscad -q -D 'part="base"'    -o stl/base.stl        enclosure_v0.scad
openscad -q -D 'part="assembly"'      -o images/front.png --imgsize=1400,900 --camera=0,0,0,70,0,-25,0 --viewall --autocenter --colorscheme=Tomorrow enclosure_v0.scad
openscad -q -D 'part="assembly_open"' -o images/inside.png --imgsize=1400,900 --camera=0,0,0,35,0,20,0 --viewall --autocenter --colorscheme=Tomorrow enclosure_v0.scad
openscad -q -D 'part="tray"'          -o images/tray.png  --imgsize=1400,900 --camera=0,0,0,75,0,-20,0 --viewall --autocenter --colorscheme=Tomorrow enclosure_v0.scad
openscad -q -D 'part="assembly"'      -o images/rear.png  --imgsize=1400,900 --camera=0,0,0,70,0,155,0 --viewall --autocenter --colorscheme=Tomorrow enclosure_v0.scad
openscad -q -D 'part="back_open"'     -o images/back_open.png --imgsize=1400,900 --camera=0,0,0,60,0,150,0 --viewall --autocenter --colorscheme=Tomorrow enclosure_v0.scad
echo done
