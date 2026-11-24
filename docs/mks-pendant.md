# MKS handheld pendant - protocol notes

The handheld pendant sold with the LS ESP32 PRO (TwoTrees / MKS, coiled RJ cable) is a
separate **ESP32** board (`ESP32_DW_MINI`, 4 MB flash) running an LVGL user interface.
It talks to the controller over **UART1** (IO17/IO18 on the controller) using
**plain-text Grbl commands** - not a binary protocol.

Source: strings extracted from the pendant firmware
`LAS_YD ESP TFT35 V4.1_V1.0.6_20240314_Bate.bin` (version string `YD_V2.0.0_20240314`)
and from the MKS controller firmware (Grbl_ESP32 based).
Baud rate is **not yet confirmed** (115200 expected) - to be measured on hardware.

## Commands sent by the pendant

| Function | Command | FluidNC status |
|---|---|---|
| Jog XY | `$J=G91 X%d Y%d F%d` | standard |
| Jog single axis | `$J=G91%c%dF%d` | standard |
| Go to XY zero | `$J=G90X0Y0F2000` | standard |
| Go to Z zero | `$J=G90Z0F1000` | standard |
| Laser / spindle | `M3 S5`, `M3 S10` ... `M3 S1000` | standard |
| Move (frame) | `G0 X%s Y%s F300`, `G1 Y%s F1000`, `G1 X%s F1000` | standard |
| Status | `?` - parses `MPos`, `WPos`, `WCO`, `Hold:0/1`, `Door:0..3`, `Alarm`, `Check`, `Sleep` | standard |
| Settings | `$%d=%d` | numeric Grbl settings |
| SD file list | `[ESP210]` | legacy ESP command - to verify |
| Run SD file | `[ESP220]<file>` | legacy ESP command - to verify |
| Frame / preview file | `[ESP801]<file>` | **MKS only** (`SD/Frame`) |
| WiFi scan | `[ESP410]` | legacy ESP command |
| WiFi settings | `$53=%s`, `$54=%s`, `$50=2` | **MKS only** |

Other strings in the pendant firmware: `Grbl_ESP32`, `GRBL V0.9` (probably used to
detect the controller from its startup banner), and the full Grbl_ESP32 error list.

## MKS-specific ESP commands in the controller firmware

| Command | Meaning |
|---|---|
| ESP210 | SD/List |
| ESP211 | BLE Start/Stop |
| ESP212 | set Z axis |
| ESP215 | SD/Delete |
| ESP220 | SD/Run |
| ESP221 | SD/Show |
| ESP801 | SD/Frame |

SD list output format: `[FILE:/%s|SIZE:%d]` (same style as FluidNC).

## MKS controller settings of interest

`uart baud rate`, `Uart1 enable`, `Probe/Enable`, `flame chack/Enable`, `safe door/Enable`,
`air cool/Enable`, `beep_status`, `Report/StallGuard`, `Current/Run`, `Current/Hold`, `Microsteps`.
The MKS firmware talks to the X/Y TMC2209 drivers over UART.

## FluidNC plan (`mks_pendant` module)

1. Pendant on `uart_channel1` (IO17/IO18).
2. Accept `[ESP801]<file>` - run a frame of the file's bounding box (or a user macro).
3. Accept MKS `$50/$53/$54` silently (no error on the pendant screen).
4. If needed, make the startup banner acceptable to the pendant.
