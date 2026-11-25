#!/usr/bin/env python3
"""
Listen to a serial port WITHOUT resetting the ESP32 (DTR/RTS kept inactive)
and log everything with timestamps, as text and hex.

Use it to watch what the MKS pendant sends, e.g. on the pendant's own USB port:

    pip install pyserial
    python tools/serial_listen.py COM6 115200 pendant_log.txt

Stop with Ctrl+C. If the text is garbage, try another baud rate
(115200, 250000, 230400, 921600, 9600).
"""

import sys
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial is missing: pip install pyserial")


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    port = sys.argv[1]
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200
    logname = sys.argv[3] if len(sys.argv) > 3 else None

    ser = serial.Serial()
    ser.port = port
    ser.baudrate = baud
    ser.timeout = 0.05
    # Keep DTR/RTS inactive BEFORE opening, so the auto-reset circuit
    # does not reboot the ESP32 we are listening to.
    ser.dtr = False
    ser.rts = False
    ser.open()

    log = open(logname, "a", encoding="utf-8") if logname else None
    t0 = time.time()
    print(f"Listening on {port} @ {baud} (Ctrl+C to stop)")
    line = b""
    last = time.time()
    try:
        while True:
            data = ser.read(256)
            now = time.time()
            if data:
                line += data
                last = now
            # flush a line on newline, or after 100 ms of silence
            while b"\n" in line or (line and now - last > 0.1):
                if b"\n" in line:
                    chunk, line = line.split(b"\n", 1)
                else:
                    chunk, line = line, b""
                chunk = chunk.rstrip(b"\r")
                text = chunk.decode("utf-8", "replace")
                printable = all(32 <= c < 127 or c in (9,) for c in chunk)
                stamp = f"{now - t0:9.3f}"
                out = f"{stamp}  {text}" if printable else f"{stamp}  {text!r}   [hex {chunk.hex(' ')}]"
                print(out)
                if log:
                    log.write(out + "\n")
                    log.flush()
    except KeyboardInterrupt:
        pass
    finally:
        ser.close()
        if log:
            log.close()


if __name__ == "__main__":
    main()
