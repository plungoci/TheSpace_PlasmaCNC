# Roadmap

## Phase 0 - Before the board arrives
- [x] Board pinout extracted from the official MKS schematic (V2.1_002)
- [x] MKS pendant command set extracted from the pendant firmware
- [x] Draft FluidNC `config.yaml` for LS ESP32 PRO V2.1
- [ ] Fork FluidNC, build the ESP32-S3 target with PlatformIO
- [ ] Pendant simulator (Python) sending the same commands as the MKS pendant
- [ ] `mks_pendant` module: `[ESP801]` handling, MKS `$50/$53/$54` settings, startup banner
- [ ] `plasma` module: `$IHS` command, Arc OK monitoring (testable on the posix build)

## Phase 1 - First power-up
- [ ] Full flash backup of the original MKS firmware (`esptool read_flash 0 0x800000`)
- [ ] Test Z driver (TMC2209 in J25) on the original firmware
- [ ] Flash FluidNC v4 (S3), load config, verify motion, limits, probe, SD card
- [ ] Confirm pendant UART baud rate, test pendant: jog -> position -> home -> SD list -> run

## Phase 2 - Plasma features on hardware
- [ ] IHS with inductive sensor (floating head)
- [ ] Torch relay on Air_assist output (IO1)
- [ ] Arc OK input on Flame input (IO36)

## Phase 3 - Advanced
- [ ] Single-wire UART to on-board TMC2209 X/Y (IO19 / IO20)
- [ ] Sensorless homing X/Y (DIAG already routed to X-/Y- via R123/R122)
- [ ] THC: arc voltage (isolated divider / module) -> ADC, Z correction, corner lock
- [ ] Optional: custom firmware for the pendant (ESP32 + LVGL)
