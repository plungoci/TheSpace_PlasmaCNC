// Copyright (c) 2026 - TheSpace PlasmaCNC contributors
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.
//
// Commands used by the MKS pendant that FluidNC does not have:
//
//   $MKS/SDList          SD root listing in the MKS format
//                        ([FILE:/name|SIZE:n] ... [SD Free:.. Used:.. Total:..])
//                        The pendant sends [ESP210]; MksPendantChannel rewrites it.
//
//   $MKS/Frame=<file>    also [ESP801]<file>   (file on the SD card)
//                        Trace the XY outline of an SD job at frame_feed_mm_per_min.
//                        Unlike the MKS laser firmware, it NEVER turns the
//                        spindle/torch on (MKS sends "M3 S5" for a laser dot).

#include "MksPendant.h"
#include "GcodeBounds.h"

#include "Settings.h"     // WebCommand, settings_execute_line
#include "FileStream.h"   // FileStream
#include "FluidPath.h"    // SD, LocalFS
#include "NutsBolts.h"    // formatBytes
#include "Driver/watchdog.h"

#include <cstdio>
#include <string>

namespace TheSpace {

    namespace {
        const char* frame_file = "mks_frame.nc";

        Error mksSDList(const char* parameter, AuthenticationLevel auth_level, Channel& out) {
            try {
                FluidPath root { "", SD };
                for (auto const& entry : stdfs::directory_iterator { root }) {
                    feed_watchdog();
                    auto name = entry.path().filename().string();
                    if (name.empty() || name[0] == '.') {
                        continue;  // hidden files, .Spotlight-V100 etc.
                    }
                    if (entry.is_directory()) {
                        continue;  // the MKS pendant only shows files in the root
                    }
                    // FluidNC's LogStream closes a line that starts with "[" with "]" itself
                    log_stream(out, "[FILE:/" << name << "|SIZE:" << entry.file_size());
                }
                auto space = stdfs::space(root);
                log_stream(out,
                           "[SD Free:" << formatBytes(space.available) << " Used:" << formatBytes(space.capacity - space.available)
                                       << " Total:" << formatBytes(space.capacity));
            } catch (std::filesystem::filesystem_error const& ex) {
                log_error_to(out, ex.what());
                return Error::FsFailedMount;
            }
            return Error::Ok;
        }

        bool scan_file(const char* path, Bounds& bounds, Channel& out) {
            BoundsScanner scanner;
            try {
                FileStream  in(path, "r", SD);
                std::string line;
                char        buf[128];
                int         n;
                size_t      lines = 0;
                while ((n = in.read(buf, sizeof(buf))) > 0) {
                    for (int i = 0; i < n; ++i) {
                        char c = buf[i];
                        if (c == '\n' || c == '\r') {
                            if (!line.empty()) {
                                scanner.feed_line(line);
                                line.clear();
                                if ((++lines & 0xff) == 0) {
                                    feed_watchdog();
                                }
                            }
                        } else {
                            line += c;
                        }
                    }
                }
                if (!line.empty()) {
                    scanner.feed_line(line);
                }
                if (in.read_failed()) {
                    log_error_to(out, "Frame: read error in " << path);
                    return false;
                }
            } catch (const ErrorException& ex) {
                log_error_to(out, "Frame: cannot open " << path);
                return false;
            }
            bounds = scanner.result();
            return true;
        }

        Error mksFrame(const char* parameter, AuthenticationLevel auth_level, Channel& out) {
            if (!parameter || !*parameter) {
                log_error_to(out, "Frame: missing file name");
                return Error::InvalidValue;
            }
            Bounds b;
            if (!scan_file(parameter, b, out)) {
                return Error::FsFailedRead;
            }
            if (!b.valid) {
                log_error_to(out, "Frame: no XY cutting moves in " << parameter);
                return Error::InvalidValue;
            }

            char text[64];
            snprintf(text, sizeof(text), "Bounds:(%.2f,%.2f), (%.2f,%.2f)", b.xmin, b.ymin, b.xmax, b.ymax);
            log_stream(out, text);

            // Write the outline as a tiny G-code job on the local filesystem and run it.
            // No M3: the torch stays off.
            try {
                FileStream f(frame_file, "w", LocalFS);
                char       g[96];
                auto       put = [&f](const char* s) { f.write(reinterpret_cast<const uint8_t*>(s), strlen(s)); };
                put("(TheSpace frame - torch stays off)\nM5\nG21 G90\n");
                snprintf(g, sizeof(g), "G0 X%.3f Y%.3f\n", b.xmin, b.ymin);
                put(g);
                snprintf(g, sizeof(g), "G1 Y%.3f F%.0f\n", b.ymax, MksPendant::frame_feed_mm_per_min);
                put(g);
                snprintf(g, sizeof(g), "G1 X%.3f\n", b.xmax);
                put(g);
                snprintf(g, sizeof(g), "G1 Y%.3f\n", b.ymin);
                put(g);
                snprintf(g, sizeof(g), "G1 X%.3f\n", b.xmin);
                put(g);
            } catch (const ErrorException& ex) {
                log_error_to(out, "Frame: cannot write " << frame_file);
                return Error::FsFailedCreateFile;
            }

            std::string cmd = std::string("$LocalFS/Run=") + frame_file;
            return settings_execute_line(cmd.c_str(), out, auth_level);
        }
    }  // namespace

    // Always-present module: the commands work from any channel (USB, telnet,
    // WebUI), with or without the pendant. Modules are initialized after the
    // built-in file commands are registered.
    class MksCommands : public Module {
    public:
        explicit MksCommands(const char* name) : Module(name) {}
        void init() override {
            new WebCommand(NULL, WEBCMD, WU, NULL, "MKS/SDList", mksSDList);
            new WebCommand("path", WEBCMD, WU, "ESP801", "MKS/Frame", mksFrame);
        }
    };

    namespace {
        ModuleFactory::InstanceBuilder<MksCommands> mks_commands_module __attribute__((init_priority(110)))("mks_commands", true);
    }

}  // namespace TheSpace
