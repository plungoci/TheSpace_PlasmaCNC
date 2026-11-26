# Stock MKS firmware backups - 2026-11-25

Full 8 MB flash dumps taken with esptool v5.6.0 before any modification.
The esptool output is kept unchanged in the `*_esptool_log.txt` files.

| File | Device | Chip | Flash | SHA-256 |
|---|---|---|---|---|
| `placa_mks_backup.bin` | MKS LS ESP32 PRO **V2.2** board (COM7) | ESP32-S3 (QFN56) rev v0.2, MAC 80:45:6b:6c:8d:74 | 8 MB quad | `b81515c2...44c7ea` |
| `telecomanda_backup.bin` | MKS handheld pendant (COM6) | ESP32-D0WD rev v1.1, MAC 58:2a:bd:6b:f2:78 | 8 MB | `c55d86aa...e1b39` |

Full hashes in `SHA256SUMS`.

## Restore

```bash
# board
python -m esptool --chip esp32s3 --port COM7 --baud 460800 write-flash 0 placa_mks_backup.bin
# pendant
python -m esptool --chip esp32 --port COM6 --baud 460800 write-flash 0 telecomanda_backup.bin
```

## Contents

### Board (`placa_mks_backup.bin`)

Partition table (identical to MKS `LAS_MKS_ESPS3 PRO V4.1_V1.0.4_20240903`):

| Name | Type | Offset | Size |
|---|---|---|---|
| nvs | data/nvs | 0x9000 | 0x5000 |
| otadata | data/ota | 0xE000 | 0x2000 |
| app0 | app/ota_0 | 0x10000 | 0x700000 (7 MB) |
| littlefs | data/spiffs | 0x710000 | 0xC8000 |
| coredump | data/coredump | 0x7D8000 | 0x10000 |

- Firmware: MKS fork of Grbl_ESP32, ESP-IDF v4.4.3, string `V4.00(8M.H35.20240226)`.
- Bootloader, partition table and littlefs are byte-identical to the MKS release file
  `LAS_MKS_ESPS3 PRO V4.1_V1.0.4_20240903_Bate.bin`; the app is a different build of the same version.
- NVS: only default values (G54-G59, G28, G30 all zero), WiFi PHY calibration and the
  Bluetooth config. **No MKS setting was ever changed** - so `$47` (screen/pendant enable)
  is still at its default (off), which explains why the pendant did not connect.
- MKS settings present in this firmware include: `Uart1 enable`, `uart baud rate`, `beep_status`,
  `Probe/Enable`, `safe door/Enable`, `flame chack/Enable`, `air cool/Enable`, `Wifi mode`,
  `Microsteps`, `Current/Run`, `Current/Hold`.

### Pendant (`telecomanda_backup.bin`)

Partition table (identical to MKS `LAS_YD ESP TFT35 V4.1_V1.0.6_20240314`):

| Name | Type | Offset | Size |
|---|---|---|---|
| nvs | data/nvs | 0x9000 | 0x5000 |
| otadata | data/ota | 0xE000 | 0x2000 |
| app0 | app/ota_0 | 0x10000 | 0x300000 (3 MB) |
| spiffs | data/spiffs | 0x310000 | 0xF0000 |

- Flash chip is 8 MB, but the MKS layout uses only the first 4 MB.
- Firmware version string: **`YD_V2.0.0_20240223`** - older than the MKS release file
  (`YD_V2.0.0_20240314`). Bootloader, partition table and spiffs are identical to that release.
- NVS keys: `screen beeper`, `MachineTheme`.

## Settings snapshot (2026-11-26)

`settings_mks_2026-11-26.txt` - output of `$I`, `$$`, `$S`, `$CMD` (tools/grbl_dump_settings.py),
taken after `$45=2` and `$47=1`. Notable values:

- `$X/Current/Run=0.250`, `$Y/Current/Run=0.250`, `Current/Hold=0.125` (A) - very low, if applied
- `$X/Microsteps=16` for all axes
- `$$` shows `$100=$101=$102=100`, but `$S` shows `Y/StepsPerMm=80`, `Z/StepsPerMm=80`
  (MKS "y/z mode steps=80" overrides Y/Z in XYZ mode)
- `$Stepper/DirInvert=XZ`, `$Limits/Invert=On`, `$Probe/Invert=On`, `$Spindle/Type=LASER`
- `$I`: `[VER:1.1h.2024022601:]`, `[MSG:Using machine:MKS DLC32]`
