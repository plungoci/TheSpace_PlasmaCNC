# Roadmap

## Phase 0 - Before the board arrives
- [x] Board pinout extracted from the official MKS schematic (V2.1_002)
- [x] MKS pendant command set extracted from the pendant firmware
- [x] Draft FluidNC `config.yaml` for LS ESP32 PRO V2.1
- [x] FluidNC v4.1.1 imported as git subtree, `thespace_s3` env builds for ESP32-S3 (see docs/building.md)
- [x] Torch on FluidNC `PlasmaSpindle` (enable + Arc OK input already supported upstream)
- [x] Pendant simulator `tools/pendant_sim.py` (serial / telnet / PC simulator)
- [x] `mks_pendant` module: MKS SD list, `[ESP801]` frame (torch off), M3 from pendant blocked, `$50/$53/$54` ignored, `SD Print Finish!`
- [x] Native unit tests (`tests/native`) + FluidNC PC simulator env `thespace_linux`
- [ ] Pendant startup banner - only if the real pendant does not connect
- [ ] `plasma` module: `$IHS` command (testable on the posix build)

## Phase 1 - First power-up
- [x] On-board X/Y TMC2209 are UART-controlled on V2.2: `$X/Current/Run` / `$Y/Current/Run` 0.25 -> 0.8 A made the motors clearly quieter (2026-11-26). Stock default 0.25 A is too low.
- [x] Pendant connects on stock MKS firmware after `$47=1` + power cycle (link icon shown) - 2026-11-25
- [x] Pendant baud = 115200 (`$46=1`, MKS manual)
- [x] Sniff pendant <-> board UART on stock firmware - boot banner, `$I`, `?` polling, jog, MPos+WPos status (docs/captures)
- [x] `mks_pendant`: drop the pendant's boot banner, send MPos+WPos in every status report
- [x] Capture SD list / run on stock firmware - pendant does not need `ok` after `[ESP220]`; progress path rewritten to MKS form
- [ ] Capture frame (`[ESP801]`) on stock firmware (optional)
- [x] Full flash backup of the original MKS firmware - board V2.2 and pendant (`backups/2026-11-25_stock_mks`)
- [ ] Test Z driver (TMC2209 in J25, SW1: 1 ON, 2 ON, 3 OFF = 16 microsteps) on the original firmware - needs `$45=2` (XYZ mode)
- [x] Flash FluidNC v4 (S3) with config + WebUI (merged image) - boots clean, config loaded, LittleFS OK, SD card OK, WebUI OK (2026-11-26, `releases/2026-11-26_first_test`)
- [x] Pendant shows the link icon on FluidNC (banner / `$I` accepted)
- [ ] Pendant on FluidNC: coordinates, jog, SD list, frame, run, laser-test button blocked
- [ ] Verify motion, limits, probe
- [ ] Confirm pendant UART baud rate, run `tools/pendant_sim.py --port ... all` on the pendant connector
- [ ] Test the real pendant: jog -> position -> home -> SD list -> frame -> run (check `[ESP220]` ok timing)

## Phase 2 - Plasma features on hardware
- [ ] IHS with inductive sensor (floating head)
- [ ] Torch relay on Air_assist output (IO1)
- [ ] Arc OK input on Flame input (IO36) - wire it and set `arc_ok_pin: gpio.36:low`

## Phase 3 - Advanced
- [ ] Single-wire UART to on-board TMC2209 X/Y (IO19 / IO20) - **needed**: confirmed on V2.2 that the drivers are configured over UART (current, microsteps); without it FluidNC cannot set their current
- [ ] Sensorless homing X/Y (DIAG already routed to X-/Y- via R123/R122)
- [ ] THC: arc voltage (isolated divider / module) -> ADC, Z correction, corner lock
- [ ] Optional: custom firmware for the pendant (ESP32 + LVGL)
