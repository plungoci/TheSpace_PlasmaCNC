// Copyright (c) 2026 - TheSpace PlasmaCNC contributors
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.
//
// MKS handheld pendant support for FluidNC.
//
// config.yaml:
//
//   uart1:
//     txd_pin: gpio.17
//     rxd_pin: gpio.18
//     baud: 115200
//     mode: 8N1
//
//   mks_pendant:
//     uart_num: 1
//     allow_spindle: false
//     ignore_mks_settings: true
//     frame_feed_mm_per_min: 3000
//     message_level: Info
//
// Do NOT also define a uart_channelN for the same UART.

#pragma once

#include "Module.h"
#include "UartChannel.h"
#include "Logging.h"

#include "MksPendantFilter.h"

namespace TheSpace {

    // A UartChannel that translates pendant lines before FluidNC executes them
    // and tells the pendant when an SD job has finished.
    class MksPendantChannel : public UartChannel {
    public:
        MksPendantChannel(objnum_t num, const PendantFilterOptions& opts) : UartChannel(num), _opts(opts) {}

        bool lineComplete(char* line, char c) override;
        void handle() override;

    private:
        const PendantFilterOptions& _opts;
        bool                        _job_was_active = false;
    };

    class MksPendant : public ConfigurableModule {
    public:
        explicit MksPendant(const char* name) : ConfigurableModule(name) {}

        void init() override;

        void group(Configuration::HandlerBase& handler) override {
            // @config uart_num
            // Which uartN: section the pendant is connected to.
            handler.item("uart_num", _uart_num, 1, MAX_N_UARTS - 1);

            // @config allow_spindle
            // Let the pendant's laser test buttons (M3/M4) through.
            // Keep false on a plasma machine: M3 fires the torch.
            handler.item("allow_spindle", _opts.allow_spindle);

            // @config ignore_mks_settings
            // Accept and ignore the MKS-only WiFi settings $50/$53/$54.
            handler.item("ignore_mks_settings", _opts.ignore_mks_settings);

            // @config frame_feed_mm_per_min
            // Feed rate used by [ESP801] (frame / trace the job outline).
            handler.item("frame_feed_mm_per_min", frame_feed_mm_per_min, 1.0f, 100000.0f);

            // @config message_level
            // Log messages sent to the pendant: None, Error, Warning, Info, Debug, Verbose.
            handler.item("message_level", _message_level, messageLevels2);
        }

        static float frame_feed_mm_per_min;

    private:
        uint32_t             _uart_num      = 1;
        uint32_t             _message_level = MsgLevelInfo;
        PendantFilterOptions _opts;
        MksPendantChannel*   _channel = nullptr;
    };

}  // namespace TheSpace
