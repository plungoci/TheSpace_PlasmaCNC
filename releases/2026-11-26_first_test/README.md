# First test build - 2026-11-26

`thespace_fluidnc_s3_merged.bin` - complete 8 MB flash image for the MKS LS ESP32 PRO
(ESP32-S3), written at offset 0:

| Offset | Content |
|---|---|
| 0x0 | bootloader |
| 0x8000 | partition table (`app3M_spiffs1M_8MB`: app0/app1 3 MB, littlefs 0x610000 size 0x1F0000) |
| 0xE000 | boot_app0 |
| 0x10000 | FluidNC v4.1.1 + TheSpace modules (mks_pendant, MKS commands) |
| 0x610000 | LittleFS: `config.yaml` (configs/ls-esp32-pro-v2.1), FluidNC WebUI |

The whole 8 MB is written, so the old MKS settings (NVS) are erased too.

## Flash

```bash
python -m esptool --chip esp32s3 --port COM4 --baud 460800 write-flash 0 thespace_fluidnc_s3_merged.bin
```

## Back to the original MKS firmware

```bash
python -m esptool --chip esp32s3 --port COM4 --baud 460800 write-flash 0 ../../backups/2026-11-25_stock_mks/placa_mks_backup.bin
```

## Notes
- Built in the development sandbox with libraries pinned from GitHub (TMCStepper v0.7.3,
  AsyncTCP v3.6.1, ESPAsyncWebServer main) instead of the PlatformIO registry.
- On-board X/Y TMC2209 are driven without UART in this build (`stepstick`), so their current
  comes from the board's VREF (MKS manual: about 1.2 A). Keep motor tests short.
- WiFi: FluidNC default access point (SSID `FluidNC`, WebUI at http://192.168.0.1).
