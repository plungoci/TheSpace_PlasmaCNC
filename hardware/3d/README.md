# MKS LS ESP32 PRO - 3D model

`LS_ESP32_PRO_V2.1_002_completed.step` is Makerbase's KiCad export
(`MKS-DLC32/MKS-ESP32-PRO-main/hardware/LS ESP32 PRO V2.1_002/`) with the parts the export
leaves out, placed from the MKS PCB drawing (`LS ESP32 PRO V2.1_002 PCB.pdf`).
Coordinates: board X -150..0, Y 0..50 mm, top copper at Z 1.65.

Added (simplified bodies, named in the STEP):

| Part | Notes |
|---|---|
| `J25_PinSocket_1x08_lower_row` | second row of the Z StepStick socket, 12.7 mm from the first |
| `TMC2209_module_PCB_on_J25` | 15.24 x 20.32 mm module sitting on the sockets |
| `TMC2209_module_heatsink_9x9x7` | top of the Z module: ~21 mm above the board |
| `X_/Y_driver_heatsink_9x11x6_assumed` | on-board driver heatsinks, size estimated from the MKS photo |
| `J14_RJ_4P4C_pendant` | pendant jack at the board edge (replaces the JST J12 of the export, which shares the footprint) |
| `J_V-TTL-s_MR30_3pin` | yellow 3-pin laser power connector, height estimated |

Measure the real board before cutting a case: the model is V2.1_002, the boards here are V2.2,
and the heights of the added parts are approximations. Regenerate with `complete_step.py`
(CadQuery / OCP).
