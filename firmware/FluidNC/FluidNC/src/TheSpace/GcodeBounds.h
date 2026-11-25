// Copyright (c) 2026 - TheSpace PlasmaCNC contributors
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.
//
// XY bounding box of a G-code program, used for "frame" (trace the outline
// of a job before cutting). Pure C++ so it can be unit tested on a PC.
//
// Only cutting moves (G1/G2/G3) count; rapids (G0) only move the position.
// Handles: G0/G1/G2/G3 (modal), G90/G91, G20/G21, I/J arcs (exact extents),
// comments "( )" and ";". Ignores axis words on G10/G28/G30/G53/G92 lines.
// R-format arcs contribute only their end point.
// Relative (G91) programs are assumed to start at X0 Y0.

#pragma once

#include <string>
#include <cstddef>

namespace TheSpace {

    struct Bounds {
        double xmin = 0, ymin = 0, xmax = 0, ymax = 0;  // mm
        bool   valid = false;                           // at least one XY cutting move seen
        size_t moves = 0;                               // number of XY cutting moves counted
    };

    class BoundsScanner {
    public:
        void   feed_line(const std::string& line);
        Bounds result() const { return _b; }

    private:
        void add_point(double x, double y);
        void add_arc(double x0, double y0, double x1, double y1, double cx, double cy, bool clockwise);

        Bounds _b;
        bool   _absolute = true;
        double _scale    = 1.0;  // 25.4 when G20 (inches)
        int    _motion   = -1;   // 0,1,2,3 or -1 (none yet)
        double _x = 0, _y = 0;   // current position, mm
    };

}  // namespace TheSpace
