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
- [ ] Full flash backup of the original MKS firmware (`esptool read_flash 0 0x800000`)
- [ ] Test Z driver (TMC2209 in J25) on the original firmware
- [ ] Flash FluidNC v4 (S3), load config, verify motion, limits, probe, SD card
- [ ] Confirm pendant UART baud rate, run `tools/pendant_sim.py --port ... all` on the pendant connector
- [ ] Test the real pendant: jog -> position -> home -> SD list -> frame -> run (check `[ESP220]` ok timing)

## Phase 2 - Plasma features on hardware
- [ ] IHS with inductive sensor (floating head)
- [ ] Torch relay on Air_assist output (IO1)
- [ ] Arc OK input on Flame input (IO36) - wire it and set `arc_ok_pin: gpio.36:low`

## Phase 3 - Advanced
- [ ] Single-wire UART to on-board TMC2209 X/Y (IO19 / IO20)
- [ ] Sensorless homing X/Y (DIAG already routed to X-/Y- via R123/R122)
- [ ] THC: arc voltage (isolated divider / module) -> ADC, Z correction, corner lock
- [ ] Optional: custom firmware for the pendant (ESP32 + LVGL)
