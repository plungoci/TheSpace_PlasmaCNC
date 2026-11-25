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

## TheSpace `mks_pendant` module

Code: `firmware/FluidNC/FluidNC/src/TheSpace/` (`MksPendant*`, `MksCommands.cpp`, `GcodeBounds*`).

### Configuration

```yaml
uart1:
  txd_pin: gpio.17
  rxd_pin: gpio.18
  baud: 115200
  mode: 8N1

mks_pendant:
  uart_num: 1
  allow_spindle: false          # block the pendant's M3/M4 "laser test" buttons
  ignore_mks_settings: true     # accept and ignore $50/$53/$54
  frame_feed_mm_per_min: 3000   # speed of [ESP801] frame
  message_level: Info
```

Do **not** define `uart_channel1` as well - the module creates its own channel on that UART
and refuses to start if one already exists.

### What the pendant channel does

| Pendant sends | Result |
|---|---|
| `[ESP210]` | rewritten to `$MKS/SDList` -> `[FILE:/name\|SIZE:n]` lines + `[SD Free:.. Used:.. Total:..]` |
| `M3 ...` / `M4 ...` (laser test buttons) | **ignored** (answer `ok` + info message) unless `allow_spindle: true` |
| `$50=`, `$53=`, `$54=` | ignored (answer `ok`) |
| `[ESP801]file` | `$MKS/Frame`: prints `Bounds:(x0,y0), (x1,y1)` and traces the outline, **torch off** |
| everything else | passed to FluidNC unchanged |

When an SD job ends, the channel sends `SD Print Finish!` (the pendant waits for this text).

### Commands available on every channel (USB, telnet, WebUI)

| Command | Description |
|---|---|
| `$MKS/SDList` | SD root listing in the MKS format |
| `$MKS/Frame=<file>` or `[ESP801]<file>` | Frame an SD file: bounding box of the cutting moves (G1/G2/G3, arcs exact), traced at `frame_feed_mm_per_min` (default 3000). Rapids (G0) are ignored, so a final `G0 X0 Y0` does not enlarge the frame. |

The frame is written to the local filesystem as `mks_frame.nc` and run as a normal job,
so it can be paused/stopped like any job. It contains no `M3`.

> Difference from the MKS laser firmware: MKS frames with `M3 S5` (a visible laser dot).
> On a plasma machine that would fire the torch, so TheSpace never does it.

### Notes for the first hardware test
- FluidNC answers `ok` to `[ESP220]` (run SD file) only **when the job ends**. The MKS
  firmware may answer immediately - check whether the pendant still shows the job screen.
- FluidNC's startup banner is `Grbl 4.x [FluidNC v4.x ...]`, not `Grbl_ESP32`. If the pendant
  does not connect, the banner is the first suspect.
- Baud rate is still unconfirmed (115200 assumed).

### Testing
- `tests/native`: unit tests for the line filter and the bounds scanner (`make -C tests/native`).
- `tools/pendant_sim.py`: sends the pendant's commands over serial, telnet or to the
  FluidNC PC simulator and checks the answers. See `tools/README.md`.
