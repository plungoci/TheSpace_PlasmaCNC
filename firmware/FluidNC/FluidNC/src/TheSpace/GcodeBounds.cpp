// Copyright (c) 2026 - TheSpace PlasmaCNC contributors
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#include "GcodeBounds.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace TheSpace {

    namespace {
        struct Word {
            char   letter;
            double value;
        };

        // Split a G-code line into words, dropping comments.
        std::vector<Word> parse_words(const std::string& line) {
            std::vector<Word> words;
            bool              in_paren = false;
            size_t            i        = 0;
            while (i < line.size()) {
                char c = line[i];
                if (in_paren) {
                    if (c == ')') {
                        in_paren = false;
                    }
                    ++i;
                    continue;
                }
                if (c == '(') {
                    in_paren = true;
                    ++i;
                    continue;
                }
                if (c == ';' || c == '%') {
                    break;
                }
                if (isalpha(static_cast<unsigned char>(c))) {
                    char letter = static_cast<char>(toupper(static_cast<unsigned char>(c)));
                    ++i;
                    while (i < line.size() && line[i] == ' ') {
                        ++i;
                    }
                    const char* start = line.c_str() + i;
                    char*       end   = nullptr;
                    double      v     = strtod(start, &end);
                    if (end == start) {
                        continue;  // letter without number (e.g. inside a word we do not know)
                    }
                    i += static_cast<size_t>(end - start);
                    words.push_back({ letter, v });
                    continue;
                }
                ++i;
            }
            return words;
        }

        bool is_code(double v, int code) { return std::fabs(v - code) < 1e-6; }
    }  // namespace

    void BoundsScanner::add_point(double x, double y) {
        if (!_b.valid) {
            _b.xmin = _b.xmax = x;
            _b.ymin = _b.ymax = y;
            _b.valid          = true;
            return;
        }
        if (x < _b.xmin) _b.xmin = x;
        if (x > _b.xmax) _b.xmax = x;
        if (y < _b.ymin) _b.ymin = y;
        if (y > _b.ymax) _b.ymax = y;
    }

    void BoundsScanner::add_arc(double x0, double y0, double x1, double y1, double cx, double cy, bool clockwise) {
        const double r = std::hypot(x0 - cx, y0 - cy);
        add_point(x1, y1);
        if (r < 1e-9) {
            return;
        }
        const double two_pi = 2.0 * M_PI;
        double       a0     = std::atan2(y0 - cy, x0 - cx);
        double       a1     = std::atan2(y1 - cy, x1 - cx);
        // Sweep from a0 to a1 counter-clockwise (positive) or clockwise
        double sweep;
        if (clockwise) {
            sweep = a0 - a1;
        } else {
            sweep = a1 - a0;
        }
        while (sweep <= 1e-9) {
            sweep += two_pi;  // same start and end point means a full circle
        }
        // Check the four axis-extreme angles 0, 90, 180, 270 degrees
        for (int k = 0; k < 4; ++k) {
            double t = k * M_PI / 2.0;
            double d = clockwise ? (a0 - t) : (t - a0);
            while (d < 0) {
                d += two_pi;
            }
            while (d >= two_pi) {
                d -= two_pi;
            }
            if (d <= sweep) {
                add_point(cx + r * std::cos(t), cy + r * std::sin(t));
            }
        }
    }

    void BoundsScanner::feed_line(const std::string& line) {
        auto words = parse_words(line);
        if (words.empty()) {
            return;
        }

        bool   non_motion_axes = false;  // G10/G28/G30/G53/G92 use axis words differently
        bool   has_x = false, has_y = false, has_i = false, has_j = false;
        double wx = 0, wy = 0, wi = 0, wj = 0;
        int    motion = _motion;

        for (const auto& w : words) {
            switch (w.letter) {
                case 'G':
                    if (is_code(w.value, 0) || is_code(w.value, 1) || is_code(w.value, 2) || is_code(w.value, 3)) {
                        motion = static_cast<int>(std::lround(w.value));
                    } else if (is_code(w.value, 90)) {
                        _absolute = true;
                    } else if (is_code(w.value, 91)) {
                        _absolute = false;
                    } else if (is_code(w.value, 20)) {
                        _scale = 25.4;
                    } else if (is_code(w.value, 21)) {
                        _scale = 1.0;
                    } else if (is_code(w.value, 10) || is_code(w.value, 28) || is_code(w.value, 30) || is_code(w.value, 53) ||
                               is_code(w.value, 92)) {
                        non_motion_axes = true;
                    } else if (is_code(w.value, 80)) {
                        motion = -1;
                    }
                    break;
                case 'X':
                    has_x = true;
                    wx    = w.value;
                    break;
                case 'Y':
                    has_y = true;
                    wy    = w.value;
                    break;
                case 'I':
                    has_i = true;
                    wi    = w.value;
                    break;
                case 'J':
                    has_j = true;
                    wj    = w.value;
                    break;
                default:
                    break;
            }
        }
        _motion = motion;

        if (non_motion_axes || motion < 0 || !(has_x || has_y)) {
            return;
        }

        const double x0 = _x;
        const double y0 = _y;
        double       x1 = _x;
        double       y1 = _y;
        if (has_x) {
            x1 = _absolute ? wx * _scale : _x + wx * _scale;
        }
        if (has_y) {
            y1 = _absolute ? wy * _scale : _y + wy * _scale;
        }

        // Only cutting moves (G1/G2/G3) define the outline. Rapids (G0) just move
        // the current position, so a final "G0 X0 Y0" park does not enlarge the frame.
        if (motion == 2 || motion == 3) {
            add_point(x0, y0);
            if (has_i || has_j) {
                add_arc(x0, y0, x1, y1, x0 + wi * _scale, y0 + wj * _scale, motion == 2);
            } else {
                add_point(x1, y1);  // R-format: end points only
            }
            ++_b.moves;
        } else if (motion == 1) {
            add_point(x0, y0);
            add_point(x1, y1);
            ++_b.moves;
        }
        _x = x1;
        _y = y1;
    }

}  // namespace TheSpace
