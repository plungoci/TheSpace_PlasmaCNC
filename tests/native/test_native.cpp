// Native (PC) unit tests for the pure parts of the TheSpace modules.
// Build & run:  make -C tests/native

#include "MksPendantFilter.h"
#include "GcodeBounds.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace TheSpace;

static int failures = 0;
static int checks   = 0;

#define CHECK(cond)                                                    \
    do {                                                               \
        ++checks;                                                      \
        if (!(cond)) {                                                 \
            ++failures;                                                \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
        }                                                              \
    } while (0)

static bool near(double a, double b) { return std::fabs(a - b) < 1e-6; }

static Bounds scan(const std::vector<std::string>& lines) {
    BoundsScanner s;
    for (auto& l : lines) {
        s.feed_line(l);
    }
    return s.result();
}

static void test_spindle_detection() {
    CHECK(line_turns_spindle_on("M3 S5"));
    CHECK(line_turns_spindle_on("m3s1000"));
    CHECK(line_turns_spindle_on("M03 S10"));
    CHECK(line_turns_spindle_on("M4"));
    CHECK(line_turns_spindle_on("G0 X1 M3"));
    CHECK(line_turns_spindle_on("M 3"));
    CHECK(!line_turns_spindle_on("M30"));
    CHECK(!line_turns_spindle_on("M300"));
    CHECK(!line_turns_spindle_on("M5"));
    CHECK(!line_turns_spindle_on("M8"));
    CHECK(!line_turns_spindle_on("G0 X10 (M3 in a comment)"));
    CHECK(!line_turns_spindle_on("G0 X10 ; M3 after semicolon"));
    CHECK(!line_turns_spindle_on("$J=G91 X10 F1000"));
    CHECK(!line_turns_spindle_on("[ESP220]/job.nc"));
    CHECK(!line_turns_spindle_on(""));
}

static void test_filter() {
    PendantFilterOptions opt;  // defaults: torch blocked, MKS settings ignored

    auto r = filter_pendant_line("[ESP210]", opt);
    CHECK(r.action == PendantAction::Rewrite);
    CHECK(r.line == "$MKS/SDList");

    r = filter_pendant_line("  [esp210]  ", opt);
    CHECK(r.action == PendantAction::Rewrite);

    r = filter_pendant_line("$50=2", opt);
    CHECK(r.action == PendantAction::Swallow);
    r = filter_pendant_line("$53=MyWifi", opt);
    CHECK(r.action == PendantAction::Swallow);
    r = filter_pendant_line("$54=secret", opt);
    CHECK(r.action == PendantAction::Swallow);
    r = filter_pendant_line("$500=1", opt);  // not an MKS setting
    CHECK(r.action == PendantAction::Pass);
    r = filter_pendant_line("$51=1", opt);
    CHECK(r.action == PendantAction::Pass);

    r = filter_pendant_line("M3 S5", opt);
    CHECK(r.action == PendantAction::Swallow);

    r = filter_pendant_line("$J=G91 X10 Y0 F1000", opt);
    CHECK(r.action == PendantAction::Pass);
    CHECK(r.line == "$J=G91 X10 Y0 F1000");

    r = filter_pendant_line("[ESP801]/part.nc", opt);
    CHECK(r.action == PendantAction::Pass);

    r = filter_pendant_line("?", opt);
    CHECK(r.action == PendantAction::Pass);

    PendantFilterOptions laser;
    laser.allow_spindle       = true;
    laser.ignore_mks_settings = false;
    r                         = filter_pendant_line("M3 S5", laser);
    CHECK(r.action == PendantAction::Pass);
    r = filter_pendant_line("$50=2", laser);
    CHECK(r.action == PendantAction::Pass);
}

static void test_bounds_lines() {
    auto b = scan({ "G21 G90", "G0 X10 Y10", "G1 X50 Y10 F1000", "G1 X50 Y30", "G1 X10 Y30", "G1 X10 Y10", "G0 X0 Y0" });
    CHECK(b.valid);
    CHECK(b.moves == 4);
    CHECK(near(b.xmin, 10) && near(b.xmax, 50));
    CHECK(near(b.ymin, 10) && near(b.ymax, 30));  // the final G0 X0 Y0 does not count
}

static void test_bounds_modal_and_comments() {
    auto b = scan({ "G90 G1 F800 X5 Y5 (start)", "X20", "Y15 ; comment X999", "(X-100 Y-100)", "X5" });
    CHECK(b.valid);
    CHECK(near(b.xmin, 0) && near(b.xmax, 20));  // first G1 starts at the unknown origin 0,0
    CHECK(near(b.ymin, 0) && near(b.ymax, 15));
}

static void test_bounds_relative_and_inches() {
    auto b = scan({ "G20 G91", "G0 X1 Y1", "G1 X2", "G1 Y1" });
    CHECK(b.valid);
    CHECK(near(b.xmin, 25.4) && near(b.xmax, 76.2));
    CHECK(near(b.ymin, 25.4) && near(b.ymax, 50.8));
}

static void test_bounds_full_circle() {
    // Full circle radius 10 around (50,50), starting at (60,50)
    auto b = scan({ "G90 G0 X60 Y50", "G2 X60 Y50 I-10 J0" });
    CHECK(b.valid);
    CHECK(near(b.xmin, 40) && near(b.xmax, 60));
    CHECK(near(b.ymin, 40) && near(b.ymax, 60));
}

static void test_bounds_quarter_arcs() {
    // CCW quarter from (10,0) to (0,10) around origin: passes no extreme in between
    auto b = scan({ "G90 G0 X10 Y0", "G3 X0 Y10 I-10 J0" });
    CHECK(near(b.xmin, 0) && near(b.xmax, 10));
    CHECK(near(b.ymin, 0) && near(b.ymax, 10));

    // CW from (10,0) to (0,10) around origin: goes the long way, through (0,-10) and (-10,0)
    b = scan({ "G90 G0 X10 Y0", "G2 X0 Y10 I-10 J0" });
    CHECK(near(b.xmin, -10) && near(b.xmax, 10));
    CHECK(near(b.ymin, -10) && near(b.ymax, 10));
}

static void test_bounds_ignored_lines() {
    auto b = scan({ "G28 X0 Y0", "G92 X100 Y100", "G53 G0 X500 Y500", "G0 X5 Y5", "G1 Z-1", "M3", "G1 X6 Y7" });
    CHECK(b.valid);
    CHECK(b.moves == 1);
    CHECK(near(b.xmin, 5) && near(b.xmax, 6));
    CHECK(near(b.ymin, 5) && near(b.ymax, 7));

    auto empty = scan({ "G0 X10 Y10", "M5", "(nothing to cut)" });
    CHECK(!empty.valid);
}

// Lines captured from the real pendant at power-up (2026-11-26)
static void test_boot_banner() {
    const char* banner[] = { "/**************************************************/",
                             "         Board Support from Makerbase",
                             "/*-Board Name:ESP32_DW_MINI",
                             "/*-Machine Axis Num:4",
                             "/*-FW Version:YD_V2.0.0_20240223" };
    PendantFilterOptions opt;
    for (auto l : banner) {
        CHECK(is_pendant_boot_banner(l));
        CHECK(filter_pendant_line(l, opt).action == PendantAction::Swallow);
    }
    // real pendant commands must pass
    CHECK(filter_pendant_line("$I", opt).action == PendantAction::Pass);
    CHECK(filter_pendant_line("$J=G91Y5F1000", opt).action == PendantAction::Pass);
    CHECK(filter_pendant_line("$J=G91X5F1000", opt).action == PendantAction::Pass);
    CHECK(!is_pendant_boot_banner("G0 X0 Y0"));
    CHECK(!is_pendant_boot_banner("$H"));
}

static void test_status_augmenter() {
    StatusAugmenter a;

    // FluidNC first report: MPos + WCO -> WPos added right after MPos
    std::string r = a.augment("<Idle|MPos:10.000,20.000,5.000|FS:0,0|WCO:1.000,2.000,0.500>");
    CHECK(r == "<Idle|MPos:10.000,20.000,5.000|WPos:9.000,18.000,4.500|FS:0,0|WCO:1.000,2.000,0.500>");

    // Later report without WCO uses the remembered offset
    r = a.augment("<Jog|MPos:11.000,20.000,5.000|FS:1000,0>");
    CHECK(r == "<Jog|MPos:11.000,20.000,5.000|WPos:10.000,18.000,4.500|FS:1000,0>");

    // WPos-only report -> MPos inserted before it
    r = a.augment("<Idle|WPos:10.000,18.000,4.500|FS:0,0>");
    CHECK(r == "<Idle|MPos:11.000,20.000,5.000|WPos:10.000,18.000,4.500|FS:0,0>");

    // MKS-style report with both: unchanged
    std::string mks = "<Idle|MPos:0.000,5.000,0.000|WPos:0.000,5.000,0.000|FS:0,0>";
    CHECK(a.augment(mks) == mks);

    // Not a status line: unchanged
    CHECK(a.augment("ok") == "ok");
    CHECK(a.augment("[MSG:INFO: hello]") == "[MSG:INFO: hello]");

    // No WCO known yet -> offset 0
    StatusAugmenter fresh;
    r = fresh.augment("<Idle|MPos:1.000,2.000,3.000|FS:0,0>");
    CHECK(r == "<Idle|MPos:1.000,2.000,3.000|WPos:1.000,2.000,3.000|FS:0,0>");
}

int main() {
    test_boot_banner();
    test_status_augmenter();
    test_spindle_detection();
    test_filter();
    test_bounds_lines();
    test_bounds_modal_and_comments();
    test_bounds_relative_and_inches();
    test_bounds_full_circle();
    test_bounds_quarter_arcs();
    test_bounds_ignored_lines();
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
