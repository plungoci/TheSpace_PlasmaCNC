// Copyright (c) 2026 - TheSpace PlasmaCNC contributors
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.
//
// TMC2209 on a single-wire UART with its OWN GPIO per driver.
//
// On the MKS LS ESP32 PRO the PDN_UART pin of each on-board TMC2209 goes through a
// series resistor to its own ESP32 pin (X: IO19, Y: IO20) - there is no shared UART bus
// and no chip select. FluidNC's tmc_2209 expects one UART (TX + RX) shared by all
// drivers, optionally with cs_pin. This driver keeps one hardware UART and, before every
// exchange with a driver, routes that UART's TX and RX to the driver's own pin
// (open-drain with pull-up, so the TMC2209 can answer on the same wire).
//
// config.yaml:
//
//   uart2:                 # reserves both pins; the drivers re-route them
//     txd_pin: gpio.19
//     rxd_pin: gpio.20
//     baud: 115200
//     mode: 8N1
//
//   axes:
//     x:
//       motor0:
//         tmc_2209_1wire:
//           uart_num: 2
//           uart_gpio: 19      # this driver's PDN_UART pin
//           addr: 3            # MS1 and MS2 are pulled high on the MKS board
//           shared_address_write_only: true   # no readback on this board (see .cpp)
//           run_amps: 0.8
//           ...                # every tmc_2209 option works the same

#pragma once

#include "Motors/TMC2209Driver.h"

namespace TheSpace {

    class TMC2209OneWire : public MotorDrivers::TMC2209Driver {
    public:
        explicit TMC2209OneWire(const char* name) : TMC2209Driver(name) {}

        void group(Configuration::HandlerBase& handler) override {
            // @config uart_gpio
            // ESP32 GPIO number wired (through a resistor) to this driver's PDN_UART pin.
            handler.item("uart_gpio", _uart_gpio, -1, 48);
            TMC2209Driver::group(handler);
        }

        // Each driver has its own wire, so two drivers at the same address are not really
        // shared: skip tmc_2209's "same UART + same address" checks.
        void validate() override { MotorDrivers::TrinamicUartDriver::validate(); }

        void afterParse() override {
            TMC2209Driver::afterParse();
            Assert(_uart_gpio >= 0, "tmc_2209_1wire needs uart_gpio:");
            // stallguard_debug reads the chip from a timer task, outside the pin routing below.
            Assert(!_stallguardDebugMode, "tmc_2209_1wire does not support stallguard_debug");
        }

        // Every path that talks to the chip goes through one of these.
        void config_motor() override;
        void debug_message() override;
        void set_disable(bool disable) override;
        void set_homing_phase(bool fastApproach) override;

    protected:
        bool set_homing_mode(bool isHoming) override;
        void config_message() override;

    private:
        int32_t _uart_gpio = -1;

        // Routes the UART to this driver's pin and holds the bus lock while alive.
        class Select {
        public:
            explicit Select(TMC2209OneWire& d);
            ~Select();
        };
    };

    // Exposed for unit tests / diagnostics: which GPIO each UART is currently routed to.
    int one_wire_current_gpio(uint32_t uart_num);

}  // namespace TheSpace
