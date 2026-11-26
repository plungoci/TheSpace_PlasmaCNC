// Copyright (c) 2026 - TheSpace PlasmaCNC contributors
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#include "MksPendantFilter.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

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

    bool is_pendant_boot_banner(const std::string& raw) {
        const std::string line = trim(raw);
        return starts_with(line, "/*") || starts_with(line, "Board Support from Makerbase");
    }

    namespace {
        std::vector<double> parse_values(const std::string& csv) {
            std::vector<double> v;
            const char*         p = csv.c_str();
            while (*p) {
                char*  end = nullptr;
                double d   = strtod(p, &end);
                if (end == p) {
                    break;
                }
                v.push_back(d);
                p = end;
                if (*p == ',') {
                    ++p;
                } else {
                    break;
                }
            }
            return v;
        }

        std::string format_values(const std::vector<double>& v) {
            std::string out;
            char        buf[32];
            for (size_t i = 0; i < v.size(); ++i) {
                snprintf(buf, sizeof(buf), "%s%.3f", i ? "," : "", v[i]);
                out += buf;
            }
            return out;
        }
    }  // namespace

    std::string StatusAugmenter::augment(const std::string& line) {
        if (line.size() < 3 || line.front() != '<' || line.back() != '>') {
            return line;
        }
        // split "<State|A:..|B:..>" into fields
        std::vector<std::string> fields;
        std::string              body = line.substr(1, line.size() - 2);
        size_t                   start = 0;
        while (true) {
            size_t bar = body.find('|', start);
            fields.push_back(body.substr(start, bar == std::string::npos ? std::string::npos : bar - start));
            if (bar == std::string::npos) {
                break;
            }
            start = bar + 1;
        }

        // Job progress: FluidNC "SD:96.45,/sd/test.nc" -> MKS "SD:96.45,/test.nc"
        for (auto& f : fields) {
            if (!starts_with(f, "SD:")) {
                continue;
            }
            size_t comma = f.find(',');
            if (comma == std::string::npos) {
                continue;
            }
            std::string path = f.substr(comma + 1);
            while (!path.empty() && path[0] == '/') {
                path.erase(0, 1);
            }
            if (starts_with(path, "sd/") || starts_with(path, "native_sd/")) {
                path.erase(0, path.find('/') + 1);
            }
            f = f.substr(0, comma + 1) + "/" + path;
        }

        int mpos = -1, wpos = -1;
        for (size_t i = 0; i < fields.size(); ++i) {
            if (starts_with(fields[i], "MPos:")) {
                mpos = static_cast<int>(i);
            } else if (starts_with(fields[i], "WPos:")) {
                wpos = static_cast<int>(i);
            } else if (starts_with(fields[i], "WCO:")) {
                _wco = parse_values(fields[i].substr(4));
            }
        }
        if ((mpos < 0) == (wpos < 0)) {
            // both present (or neither): nothing to add, but the SD path may have changed
            std::string out = "<";
            for (size_t i = 0; i < fields.size(); ++i) {
                out += (i ? "|" : "") + fields[i];
            }
            return out + ">";
        }

        const bool          have_m = mpos >= 0;
        std::vector<double> pos    = parse_values(fields[have_m ? mpos : wpos].substr(5));
        std::vector<double> other(pos.size());
        for (size_t i = 0; i < pos.size(); ++i) {
            double off = i < _wco.size() ? _wco[i] : 0.0;
            other[i]   = have_m ? pos[i] - off : pos[i] + off;
        }
        if (have_m) {
            fields.insert(fields.begin() + mpos + 1, "WPos:" + format_values(other));
        } else {
            fields.insert(fields.begin() + wpos, "MPos:" + format_values(other));
        }

        std::string out = "<";
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i) {
                out += '|';
            }
            out += fields[i];
        }
        return out + ">";
    }

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

        // The pendant prints its own debug banner on the link when it powers up
        if (is_pendant_boot_banner(line)) {
            return { PendantAction::Swallow, "", "pendant boot banner" };
        }

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
