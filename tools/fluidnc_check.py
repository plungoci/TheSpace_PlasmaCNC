#!/usr/bin/env python3
"""
First-boot check for FluidNC: sends a few diagnostic commands and saves
everything to a text file.

    pip install pyserial
    python tools/fluidnc_check.py COM4 fluidnc_check.txt

Commands sent:
  $SS              startup log (shows [MSG:ERR...] lines from boot)
  $I               build info
  $CD              the configuration FluidNC actually loaded
  $LocalFS/List    files on the board (config.yaml, WebUI)
  $MKS/SDList      SD card list in MKS format (TheSpace module)
  ?                status report
Lines containing ERR / error / WARN are repeated in a summary at the end.
"""

import sys
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial is missing: pip install pyserial")

COMMANDS = ["$SS", "$I", "$CD", "$LocalFS/List", "$MKS/SDList", "?"]


def read_until_idle(ser, idle=1.0, limit=15.0):
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
    outname = sys.argv[2] if len(sys.argv) > 2 else "fluidnc_check.txt"

    ser = serial.Serial(port, 115200, timeout=0.1)
    print(f"Connected to {port}, waiting for the board...")
    boot = read_until_idle(ser, idle=2.0, limit=10)  # opening the port may reboot the board

    problems = []
    with open(outname, "w", encoding="utf-8") as f:
        f.write(f"# FluidNC check on {port} - {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
        if boot.strip():
            f.write("\n## output after connecting\n" + boot.strip() + "\n")
        for cmd in COMMANDS:
            if cmd == "?":
                ser.write(b"?")
            else:
                ser.write(cmd.encode() + b"\n")
            text = read_until_idle(ser)
            f.write(f"\n## {cmd}\n{text.strip()}\n")
            print(f"{cmd:14s} -> {len(text.strip().splitlines())} lines")
        f.seek(0)
    with open(outname, encoding="utf-8") as f:
        for line in f:
            if any(k in line for k in ("ERR", "error", "WARN", "Error")):
                problems.append(line.rstrip())
    with open(outname, "a", encoding="utf-8") as f:
        f.write("\n## summary: lines with ERR / error / WARN\n")
        f.write("\n".join(problems) if problems else "(none)")
        f.write("\n")
    ser.close()

    print(f"\nSaved to {outname}")
    if problems:
        print("Lines with ERR / error / WARN:")
        for p in problems:
            print("  " + p)
    else:
        print("No ERR / error / WARN lines.")


if __name__ == "__main__":
    main()
