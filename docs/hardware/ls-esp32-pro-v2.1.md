# MKS LS ESP32 PRO V2.1 - Pinout

Source: official Makerbase schematic `LS ESP32 PRO V2.1_002 SCH.pdf` (2023-11-21),
from the [makerbase-mks/MKS-DLC32](https://github.com/makerbase-mks/MKS-DLC32) repository,
folder `MKS-ESP32-PRO-main/hardware`.

MCU module: **ESP32-S3-WROOM-1U-N8** (8 MB flash). Older V1.1 boards use a classic ESP32
and are **not** covered here.

> Requires FluidNC **v4.x** (first version with ESP32-S3 support).
> No Bluetooth on S3 builds. Only the first 4 MB of flash are used by FluidNC.

## GPIO map

| GPIO | Signal | Notes |
|---|---|---|
| IO1 | Air_assist | MOSFET output on J4 - planned torch relay |
| IO2 | LC | TTL output (laser control) |
| IO4 | Z_DIR | to Z socket J25 / header J26 |
| IO5 | Z_STEP | to Z socket J25 / header J26 |
| IO6 | Y_DIR | on-board TMC2209 (U8) |
| IO7 | Y_STEP | on-board TMC2209 (U8) |
| IO8 | XYZ_EN | shared enable for X, Y, Z (TMC2209 ENN, high = disabled) |
| IO9 | SD_VCC_SELECT | SD supply 1.8/3.3 V; pull-down -> 3.3 V default. Leave unused. |
| IO10 | SD_DET | SD card detect |
| IO11 | SD_D1 | |
| IO12 | SD_D0 | MISO in SPI mode |
| IO13 | SD_CLK | SCK |
| IO14 | SD_CMD | MOSI in SPI mode |
| IO15 | X_DIR | on-board TMC2209 (U7) |
| IO16 | X_STEP | on-board TMC2209 (U7) |
| IO17 | TXD1 | pendant UART TX (J12/J14) |
| IO18 | RXD1 | pendant UART RX (J12/J14) |
| IO19 | X_UART_CS | single-wire UART to PDN_UART of X TMC2209 |
| IO20 | Y_UART_CS | single-wire UART to PDN_UART of Y TMC2209 |
| IO21 | SD_D3 | CS in SPI mode |
| IO35 | PWM | laser PWM output |
| IO36 | Flame | protected input (J17) - planned **Arc OK** |
| IO37 | Probe | protected input (J18) - **IHS sensor** |
| IO38 | Door | protected input (J16) |
| IO39 | X- | limit input (J7) |
| IO40 | Y- | limit input (J8) |
| IO41 | Z- | limit input (J9) |
| IO42 | BEEPER | buzzer (J13) |
| IO47 | SD_D2 | |
| IO0 | BOOT | button |
| IO3, IO45, IO46 | - | strapping pins, avoid |
| IO48 | - | free on the module, unprotected |
| TXD0 / RXD0 | USB serial | console |

## Connectors

### Pendant - J12 (header) / J14 (RJ)
| Pin | Signal |
|---|---|
| 1 | +5V (via ferrite L4) |
| 2 | GND (via ferrite L5) |
| 3 | TXD1 (IO17, series R52, TVS D18) |
| 4 | RXD1 (IO18, series R53, TVS D19) |

### Probe J18 / Flame J17 / Door J16
| Pin | Signal |
|---|---|
| 1 | +5V |
| 2 | GND |
| 3 | Signal - series resistor, RC filter, clamp diodes, pull-up to 3.3 V |

An NPN NO inductive sensor can be connected directly (output to pin 3, active low).
Most inductive sensors need 6-36 V: power the sensor from the 12-24 V supply, not from pin 1,
and keep a common GND.

### Limits J7 (X-) / J8 (Y-) / J9 (Z-)
Pull-up to 3.3 V, RC filter, diode D14/D15/D16 from the connector side.
TMC2209 DIAG outputs of X and Y are tied to the X-0 / Y-0 nets through R123 / R122
(check whether these resistors are fitted). DIAG idles low and goes high on stall,
so with DIAG connected the limit input reads **low normally, high on stall**.

### Z driver - J25 (StepStick socket) / J26 (header)
- J25: StepStick socket for an external TMC2209 module, motor on J27.
  Microstepping via DIP switch SW1 (MS1, MS2, MS3).
- J26: EN / STEP / DIR / GND for an external driver (DM542, TB6600...).
- DIAG and UART of the Z driver are **not** connected to the MCU.

TMC2209 standalone microstep table:

| MS2 | MS1 | Microsteps |
|---|---|---|
| 0 | 0 | 8 |
| 0 | 1 | 32 |
| 1 | 0 | 64 |
| 1 | 1 | 16 |

On many TMC2209 modules the StepStick "MS3" position is the UART pin - keep SW1 MS3 **OFF**.

## Power
- Input 12-24 V (J2/J3), 5 V buck (MP1584EN), 3.3 V LDO (AMS1117).
