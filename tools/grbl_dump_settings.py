#!/usr/bin/env python3
"""
Save all settings of a Grbl_ESP32 / MKS / FluidNC controller to a text file.

    pip install pyserial
    python tools/grbl_dump_settings.py COM7 settings_mks.txt

Sends: $I (build info), $$ (numbered Grbl settings), $S (all named settings,
incl. driver microsteps/current on Grbl_ESP32/MKS), $CMD (command list).
Commands that the firmware does not know are simply logged with their error.
"""

import sys
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial is missing: pip install pyserial")

COMMANDS = ["$I", "$$", "$S", "$CMD"]


def read_until_idle(ser, idle=1.0, limit=20.0):
    out = b""
    start = last = time.time()
    while time.time() - start < limit:
        data = ser.read(4096)
        if data:
            out += data
            last = time.time()
        elif time.time() - last > idle:
            break
    return out.decode("utf-8", "replace")


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    port = sys.argv[1]
    outname = sys.argv[2] if len(sys.argv) > 2 else "settings.txt"
    baud = int(sys.argv[3]) if len(sys.argv) > 3 else 115200

    ser = serial.Serial(port, baud, timeout=0.1)
    print(f"Connected to {port}, waiting for the controller...")
    banner = read_until_idle(ser, idle=1.5, limit=6)  # the board may reboot on connect

    with open(outname, "w", encoding="utf-8") as f:
        f.write(f"# Settings dump from {port} @ {baud} - {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
        if banner.strip():
            f.write("\n## startup\n" + banner.strip() + "\n")
        for cmd in COMMANDS:
            ser.write(cmd.encode() + b"\n")
            text = read_until_idle(ser)
            f.write(f"\n## {cmd}\n{text.strip()}\n")
            lines = len(text.strip().splitlines())
            print(f"{cmd:5s} -> {lines} lines")
    ser.close()
    print(f"Saved to {outname}")


if __name__ == "__main__":
    main()
