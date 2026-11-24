# IHS - Initial Height Sensing

## Why not inductive THC
An inductive proximity sensor is **not** suitable for height control during the cut:
- detects ferrous metal only; range drops strongly on stainless / aluminium
- mounted beside the torch, so it misreads over holes, edges and cut-out parts
- heat, spatter and HF arc-start noise disturb it

It works well for **IHS** - finding the plate before each pierce. Real THC uses arc voltage
(see roadmap, phase 3).

## Floating head (recommended)
The torch sits on a small slide that can move up a few mm. The inductive sensor is fixed on the
Z carriage and detects a steel flag on the torch. When the torch touches the plate, it is pushed
up and the sensor triggers the probe input. Works on **any** material, including aluminium.

## Wiring
- NPN NO sensor, powered from 12-24 V
- Output -> J18 pin 3 (Probe, IO37), common GND
- Board input already has pull-up, RC filter and clamp diodes
- Shielded cable, shield to GND at one end, ferrites - plasma HF noise easily resets an ESP32
- Optional extra isolation: optocoupler (PC817) between sensor and input

## G-code (current, before the `$IHS` command exists)
Example with 1.5 mm floating-head switch travel:

```gcode
G38.2 Z-60 F800      ; fast probe down
G91 G0 Z2            ; back off
G90
G38.2 Z-5 F150       ; slow probe
G92 Z-1.5            ; compensate floating-head travel -> Z0 = plate surface
G0 Z3.8              ; pierce height
M3                   ; torch on
G4 P0.6              ; pierce delay
G1 Z1.5 F1000        ; cut height
...                  ; contour
M5
G0 Z15
```

Pierce height, cut height and pierce delay come from the plasma source cut chart.
This block has to be inserted by the CAM post-processor before every pierce.
The planned `$IHS` command will replace it with a single line.
