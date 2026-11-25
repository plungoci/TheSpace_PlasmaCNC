// Copyright (c) 2026 - TheSpace PlasmaCNC contributors
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#include "MksPendant.h"

#include "Machine/MachineConfig.h"  // config
#include "Serial.h"                 // allChannels
#include "Job.h"                    // Job::active()

#include <cstring>

namespace TheSpace {

    float MksPendant::frame_feed_mm_per_min = 3000.0f;

    // ---------------------------------------------------------------------
    // Channel
    // ---------------------------------------------------------------------

    bool MksPendantChannel::lineComplete(char* line, char c) {
        if (!UartChannel::lineComplete(line, c)) {
            return false;
        }
        PendantResult r = filter_pendant_line(line, _opts);
        switch (r.action) {
            case PendantAction::Pass:
                break;
            case PendantAction::Rewrite:
                log_debug("Pendant: " << line << " -> " << r.line << " (" << r.reason << ")");
                strncpy(line, r.line.c_str(), Channel::maxLine - 1);
                line[Channel::maxLine - 1] = '\0';
                break;
            case PendantAction::Swallow:
                log_info_to(*this, "Pendant: ignored " << line << " (" << r.reason << ")");
                line[0] = '\0';  // an empty line is acknowledged with "ok"
                break;
        }
        return true;
    }

    void MksPendantChannel::handle() {
        UartChannel::handle();

        // The MKS pendant leaves its "printing" screen when it sees this text.
        bool active = Job::active();
        if (_job_was_active && !active) {
            log_stream(*this, "SD Print Finish!");
        }
        _job_was_active = active;
    }

    // ---------------------------------------------------------------------
    // Module
    // ---------------------------------------------------------------------

    void MksPendant::init() {
        if (config->_uart_channels[_uart_num]) {
            log_error("mks_pendant: uart_channel" << _uart_num << " is also defined - remove it, the pendant needs its own channel");
            return;
        }
        auto uart = config->_uarts[_uart_num];
        if (!uart) {
            log_error("mks_pendant: missing uart" << _uart_num << " section");
            return;
        }
        _channel                 = new MksPendantChannel(_uart_num, _opts);
        _channel->_message_level = _message_level;
        _channel->init(uart);  // registers the channel with allChannels

        log_info("MKS pendant on uart" << _uart_num << " torch from pendant:" << (_opts.allow_spindle ? "allowed" : "blocked")
                                       << " frame feed:" << frame_feed_mm_per_min);
    }

    // Configuration registration
    namespace {
        ConfigurableModuleFactory::InstanceBuilder<MksPendant> registration("mks_pendant");
    }

}  // namespace TheSpace
