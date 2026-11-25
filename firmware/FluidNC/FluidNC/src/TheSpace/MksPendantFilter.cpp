// Copyright (c) 2026 - TheSpace PlasmaCNC contributors
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#include "MksPendantFilter.h"

#include <cctype>

namespace TheSpace {

    namespace {
        std::string trim(const std::string& s) {
            size_t b = 0;
            size_t e = s.size();
            while (b < e && isspace(static_cast<unsigned char>(s[b]))) {
                ++b;
            }
            while (e > b && isspace(static_cast<unsigned char>(s[e - 1]))) {
                --e;
            }
            return s.substr(b, e - b);
        }

        std::string upper(std::string s) {
            for (auto& c : s) {
                c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
            }
            return s;
        }

        bool starts_with(const std::string& s, const char* prefix) {
            return s.rfind(prefix, 0) == 0;
        }

        // "$50", "$53=abc", "$54 = x" -> true for the MKS WiFi settings
        bool is_mks_setting(const std::string& u) {
            if (!starts_with(u, "$5")) {
                return false;
            }
            if (u.size() < 3) {
                return false;
            }
            char d = u[2];
            if (d != '0' && d != '3' && d != '4') {
                return false;
            }
            // must not be followed by another digit ($500 is not an MKS setting)
            return u.size() == 3 || !isdigit(static_cast<unsigned char>(u[3]));
        }
    }  // namespace

    bool line_turns_spindle_on(const std::string& line) {
        const std::string u = upper(line);
        if (u.empty() || u[0] == '$' || u[0] == '[') {
            return false;  // not G-code
        }
        bool in_paren = false;
        for (size_t i = 0; i < u.size(); ++i) {
            char c = u[i];
            if (in_paren) {
                if (c == ')') {
                    in_paren = false;
                }
                continue;
            }
            if (c == '(') {
                in_paren = true;
                continue;
            }
            if (c == ';') {
                break;
            }
            if (c != 'M') {
                continue;
            }
            // Read the number after M, skipping spaces and leading zeros
            size_t j = i + 1;
            while (j < u.size() && u[j] == ' ') {
                ++j;
            }
            size_t start = j;
            while (j < u.size() && isdigit(static_cast<unsigned char>(u[j]))) {
                ++j;
            }
            if (j == start) {
                continue;
            }
            std::string num = u.substr(start, j - start);
            size_t      nz  = num.find_first_not_of('0');
            num             = (nz == std::string::npos) ? "0" : num.substr(nz);
            if (num == "3" || num == "4") {
                // M3.5 style subcodes do not exist for M3/M4; a '.' means it is not M3/M4
                if (j < u.size() && u[j] == '.') {
                    continue;
                }
                return true;
            }
        }
        return false;
    }

    PendantResult filter_pendant_line(const std::string& raw, const PendantFilterOptions& opt) {
        const std::string line = trim(raw);
        const std::string u    = upper(line);

        // MKS SD file list -> same listing in the MKS format
        if (u == "[ESP210]") {
            return { PendantAction::Rewrite, "$MKS/SDList", "MKS SD list format" };
        }

        if (opt.ignore_mks_settings && is_mks_setting(u)) {
            return { PendantAction::Swallow, "", "MKS-only setting ignored" };
        }

        if (!opt.allow_spindle && line_turns_spindle_on(line)) {
            return { PendantAction::Swallow, "", "torch/spindle on from pendant is disabled" };
        }

        return { PendantAction::Pass, line, nullptr };
    }

}  // namespace TheSpace
