# TMC2209 UART build - 2026-11-26

Adds `tmc_2209_1wire`: FluidNC talks to the on-board X/Y TMC2209 over single-wire UART
(X: IO19, Y: IO20, address 3). Current, microsteps and StealthChop/SpreadCycle now come from
`config.yaml`.

| File | Use |
|---|---|
| `thespace_fluidnc_s3_app.bin` | firmware only, write at `0x10000` - keeps the files and settings on the board |
| `config.yaml` | new board config (X/Y as `tmc_2209_1wire`, `uart2`) - upload it with the WebUI |
| `thespace_fluidnc_s3_merged.bin` | full 8 MB image (config.yaml + TheSpace WebUI + FluidNC WebUI), erases everything |

## Update keeping the board files

```bash
python -m esptool --chip esp32s3 --port COM4 --baud 460800 write-flash 0x10000 thespace_fluidnc_s3_app.bin
```

Then copy your calibrated `steps_per_mm` into `config.yaml`, upload it (Setup -> Board files),
and restart (`$Bye`). Upload the config **after** the firmware: the old firmware does not know
`tmc_2209_1wire` and would not load it.

## Check

`$SS` (or the console at boot) should show for X and Y:

```
[MSG:INFO:    x motor0 1-wire UART2 GPIO19 Addr:3 Step:gpio.16 ...]
[MSG:INFO:    x motor0 driver test passed]
```

`driver test failed` means no UART answer: check the address (0-3) and that the build is this one.

## Notes
- `run_amps: 0.8`, `hold_amps: 0.4`, StealthChop (quiet, like Z). If the motors are weak at
  speed, try `run_mode: CoolStep` (SpreadCycle) or more current, e.g. `$/axes/x/motor0/tmc_2209_1wire/run_amps=1.0`.
- Both Y connectors share one driver, so 0.8 A is split between the two Y motors.
- Microsteps are 16, the same as the board's standalone setting, so `steps_per_mm` should not change.
  Check with the calibration tool anyway.
- Save runtime changes with `$CD=config.yaml`.
