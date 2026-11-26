// Copyright (c) 2026 - TheSpace PlasmaCNC contributors
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

// Trinamic drivers (TMCStepper) exist only in the ESP32 builds, not in the Linux simulator.
#if defined(ESP_PLATFORM)

#include "TMC2209OneWire.h"

#include "Machine/MachineConfig.h"  // MotorFactory
#include "Logging.h"

#include <mutex>

#if defined(ESP_PLATFORM)
#    include "Driver/fluidnc_uart.h"  // uart_pins()
#    include "driver/gpio.h"
#    include "esp_rom_gpio.h"
#    include "soc/gpio_sig_map.h"
#    include "soc/gpio_struct.h"
#    include "hal/gpio_ll.h"
#endif

namespace TheSpace {

    namespace {
        // One lock for all one-wire drivers: they share the UART engine.
        std::recursive_mutex bus_lock;
        constexpr int        MAX_UARTS = 4;
        int                  routed[MAX_UARTS] = { -1, -1, -1, -1 };

        void release_pin(int gpio) {
#if defined(ESP_PLATFORM)
            // Disconnect the UART and hold the line high (UART idle level), like the MKS
            // firmware does. The board has a 20k pull-down on PDN_UART, so a released pin
            // would sit low and look like a break to the driver.
            esp_rom_gpio_connect_out_signal(gpio, SIG_GPIO_OUT_IDX, false, false);
            gpio_ll_od_disable(&GPIO, uint32_t(gpio));
            gpio_set_level(gpio_num_t(gpio), 1);
            gpio_set_direction(gpio_num_t(gpio), GPIO_MODE_OUTPUT);
#endif
        }

        void route(uint32_t uart_num, int gpio) {
            if (uart_num >= MAX_UARTS || routed[uart_num] == gpio) {
                return;
            }
#if defined(ESP_PLATFORM)
            if (routed[uart_num] >= 0) {
                release_pin(routed[uart_num]);
            }
            // Push-pull TX through the board's series resistor (R124/...), RX on the same pad.
            // Open-drain does not work here: the 20k pull-down on PDN_UART beats the ESP32's
            // weak internal pull-up, so the line never reaches a valid high level.
            // Without an open-drain line the driver cannot answer, so the drivers are used
            // write-only (shared_address_write_only: true in config.yaml).
            uart_pins(uart_num, pinnum_t(gpio), pinnum_t(gpio), pinnum_t(-1), pinnum_t(-1));
            gpio_ll_od_disable(&GPIO, uint32_t(gpio));
#endif
            routed[uart_num] = gpio;
        }

        uint32_t first_route_uart = MAX_UARTS;  // uart whose config pins we must release once
    }  // namespace

    int one_wire_current_gpio(uint32_t uart_num) { return uart_num < MAX_UARTS ? routed[uart_num] : -1; }

    TMC2209OneWire::Select::Select(TMC2209OneWire& d) {
        bus_lock.lock();
        uint32_t n = uint32_t(d._uart_num);
        if (n < MAX_UARTS && routed[n] < 0 && first_route_uart != n) {
            // The first time, the UART still sits on the pins of its uartN: section
            // (TX on one driver's pin as push-pull, RX on the other). Release both.
            first_route_uart = n;
#if defined(ESP_PLATFORM)
            auto* u = config->_uarts[n];
            if (u) {
                for (Pin* p : { &u->_txd_pin, &u->_rxd_pin }) {
                    if (!p->undefined()) {
                        release_pin(p->getNative(Pin::Capabilities::Input));
                    }
                }
            }
#endif
        }
        route(n, d._uart_gpio);
    }

    TMC2209OneWire::Select::~Select() { bus_lock.unlock(); }

    void TMC2209OneWire::config_motor() {
        Select s(*this);
        TMC2209Driver::config_motor();
    }

    void TMC2209OneWire::debug_message() {
        Select s(*this);
        TMC2209Driver::debug_message();
    }

    void TMC2209OneWire::set_disable(bool disable) {
        Select s(*this);
        TMC2209Driver::set_disable(disable);
    }

    void TMC2209OneWire::set_homing_phase(bool fastApproach) {
        Select s(*this);
        TMC2209Driver::set_homing_phase(fastApproach);
    }

    bool TMC2209OneWire::set_homing_mode(bool isHoming) {
        Select s(*this);
        return TMC2209Driver::set_homing_mode(isHoming);
    }

    void TMC2209OneWire::config_message() {
        log_info("    " << name() << " 1-wire UART" << _uart_num << " GPIO" << _uart_gpio << " Addr:" << _addr << " Step:" << _step_pin.name()
                        << " Dir:" << _dir_pin.name() << " Disable:" << _disable_pin.name() << " R:" << _r_sense);
    }

    // Configuration registration
    namespace {
        MotorDrivers::MotorFactory::InstanceBuilder<TMC2209OneWire> registration("tmc_2209_1wire");
    }

}  // namespace TheSpace

#endif  // ESP_PLATFORM
