# Building the firmware

The firmware lives in `firmware/FluidNC` - upstream [FluidNC](https://github.com/bdring/FluidNC)
imported as a **git subtree** (squashed), currently at **v4.1.1**.

Project-specific build environments are in `firmware/FluidNC/thespace.ini`
(included from `platformio.ini` -> `extra_configs`). That one line is the only change to an
upstream file. Our code is in `firmware/FluidNC/FluidNC/src/TheSpace/` - a folder upstream
does not have, so `git subtree pull` never conflicts with it.

## Requirements

- [PlatformIO](https://platformio.org/) (CLI or the VS Code extension)
- Internet access to the PlatformIO registry and GitHub (first build downloads ~1 GB of toolchain)

## Build

```bash
cd firmware/FluidNC
pio run -e thespace_s3
```

Output in `firmware/FluidNC/.pio/build/thespace_s3/` (`firmware.bin`, plus a merged image
produced by `build_merged.py`).

Last verified build (2026-11-25, with the mks_pendant module): RAM 36.6 %, Flash 69.2 % of the 3 MB app partition.

## Unit tests (no hardware, no PlatformIO)

```bash
make -C tests/native
```

Tests the pendant line filter and the G-code bounds scanner with the host C++ compiler.

## PC simulator (Linux / WSL)

FluidNC can run as a normal program on a PC. Our modules are included.

```bash
cd firmware/FluidNC
pio run -e thespace_linux
cd ../../tools/sim
../../firmware/FluidNC/.pio/build/thespace_linux/program -      # type commands, e.g. $MKS/Frame=part.nc
python ../pendant_sim.py --exec ../../firmware/FluidNC/.pio/build/thespace_linux/program all frame --file part.nc
```

The simulator reads `native_localfs/config.yaml` and uses `native_sd/` as the SD card.

## Flash

Before the first flash, back up the original MKS firmware (8 MB):

```bash
esptool.py --chip esp32s3 -p COMx read_flash 0 0x800000 backup_mks.bin
```

Then:

```bash
pio run -e thespace_s3 -t upload
```

Upload `configs/ls-esp32-pro-v2.1/config.yaml` to the board's local filesystem
(WebUI or `$LocalFS/...` commands), named `config.yaml`.

## Updating FluidNC

```bash
git subtree pull --prefix=firmware/FluidNC https://github.com/bdring/FluidNC <tag> --squash
```

Our changes are kept in separate files, so conflicts should be limited to `platformio.ini`
(the `thespace.ini` line in `extra_configs`).

## Useful upstream features

- `PlasmaSpindle` (experimental): torch enable output + `arc_ok_pin` + `arc_wait_ms`.
  Arc OK loss while cutting raises an alarm and aborts motion. Used in our config.
