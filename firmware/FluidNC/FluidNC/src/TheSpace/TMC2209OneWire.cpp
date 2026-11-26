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
            // Disconnect the UART TX signal and leave the line as a pulled-up input.
            esp_rom_gpio_connect_out_signal(gpio, SIG_GPIO_OUT_IDX, false, false);
            gpio_set_direction(gpio_num_t(gpio), GPIO_MODE_INPUT);
            gpio_pullup_en(gpio_num_t(gpio));
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
            // Direction first (it resets the pad's output source), then connect the UART,
            // then make the pad open-drain with pull-up so the TMC2209 can drive the line.
            gpio_set_direction(gpio_num_t(gpio), GPIO_MODE_INPUT_OUTPUT_OD);
            uart_pins(uart_num, pinnum_t(gpio), pinnum_t(gpio), pinnum_t(-1), pinnum_t(-1));
            gpio_ll_od_enable(&GPIO, uint32_t(gpio));  // open-drain without touching the signal routing
            gpio_pullup_en(gpio_num_t(gpio));
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
