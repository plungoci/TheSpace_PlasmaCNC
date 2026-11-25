// Copyright (c) 2026 - TheSpace PlasmaCNC contributors
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.
//
// Translation of lines sent by the MKS handheld pendant into commands that
// FluidNC understands. Pure C++ (no FluidNC dependencies) so it can be unit
// tested on a PC.

#pragma once

#include <string>

namespace TheSpace {

    struct PendantFilterOptions {
        // The pendant has "laser test" buttons that send M3 S5 ... M3 S1000.
        // On a plasma machine M3 fires the torch, so this is OFF by default.
        bool allow_spindle = false;

        // MKS-only WiFi settings ($50, $53, $54) are accepted and ignored,
        // so the pendant does not show an error.
        bool ignore_mks_settings = true;
    };

    enum class PendantAction {
        Pass,     // forward the line unchanged
        Rewrite,  // forward the rewritten line
        Swallow,  // do not execute; the channel still answers "ok"
    };

    struct PendantResult {
        PendantAction action;
        std::string   line;    // line to execute (Pass/Rewrite), empty for Swallow
        const char*   reason;  // short explanation for Rewrite/Swallow, nullptr for Pass
    };

    // Translate one complete line received from the pendant.
    PendantResult filter_pendant_line(const std::string& line, const PendantFilterOptions& opt);

    // True if the line is G-code containing M3 or M4 (spindle / torch on).
    // M30, M300 etc. are not matched. Comments are ignored.
    bool line_turns_spindle_on(const std::string& line);

}  // namespace TheSpace
