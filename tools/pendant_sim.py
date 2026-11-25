#!/usr/bin/env python3
"""
MKS pendant simulator - TheSpace PlasmaCNC

Sends the same commands as the MKS handheld pendant (TwoTrees / MKS, RJ cable)
and checks the answers, so the controller side can be tested without the pendant.

Transports:
  --port COM5 / /dev/ttyUSB0   serial (USB-UART on the pendant connector J12/J14,
                               or the board's USB port)            needs: pip install pyserial
  --tcp HOST[:PORT]            telnet to FluidNC over WiFi (default port 23)
  --exec "PROGRAM"             start the FluidNC PC simulator (pio run -e thespace_linux)
                               and talk to it over stdin/stdout; run it in a folder with
                               native_localfs/config.yaml and native_sd/<files>

Note: the pendant-specific translation ([ESP210] -> MKS list, blocked M3, ignored $50/$53/$54)
only happens on the pendant UART channel (mks_pendant: in config.yaml). Over USB or telnet
those lines reach FluidNC unchanged, so use --port on the pendant connector to test them.

Examples:
  python pendant_sim.py --tcp 192.168.1.50 status jog list
  python pendant_sim.py --port COM5 --baud 115200 all
  python pendant_sim.py --port COM5 frame --file part.nc
  python pendant_sim.py --tcp 127.0.0.1 send "$MKS/SDList"
"""

import argparse
import queue
import re
import shlex
import socket
import subprocess
import sys
import threading
import time

# Commands taken from the MKS pendant firmware (LAS_YD ESP TFT35 V4.1_V1.0.6_20240314)
PENDANT = {
    "status": "?",
    "unlock": "$X",
    "home": "$H",
    "jog_x+": "$J=G91X10F1000",
    "jog_x-": "$J=G91X-10F1000",
    "jog_xy": "$J=G91 X5 Y5 F1000",
    "jog_z+": "$J=G91Z1F300",
    "goto_xy0": "$J=G90X0Y0F2000",
    "goto_z0": "$J=G90Z0F1000",
    "laser_test": "M3 S5",
    "list": "[ESP210]",
    "run": "[ESP220]{file}",
    "frame": "[ESP801]{file}",
    "wifi_mode": "$50=2",
    "wifi_ssid": "$53=TestSSID",
    "wifi_pass": "$54=TestPass",
}

STATUS_RE = re.compile(r"<(?P<state>[A-Za-z]+)(:\d)?\|(?P<pos>[MW]Pos):(?P<x>-?[\d.]+),(?P<y>-?[\d.]+),(?P<z>-?[\d.]+)")
FILE_RE = re.compile(r"\[FILE:/?(?P<name>[^|\]]+)\|SIZE:(?P<size>\d+)")
BOUNDS_RE = re.compile(r"Bounds:\((?P<x0>-?[\d.]+),(?P<y0>-?[\d.]+)\), \((?P<x1>-?[\d.]+),(?P<y1>-?[\d.]+)\)")


class Link:
    """Line-oriented connection over serial or TCP."""

    def __init__(self, args):
        self.buf = b""
        self.verbose = args.verbose
        self.proc = None
        if args.exec_cmd:
            self.proc = subprocess.Popen(shlex.split(args.exec_cmd) + ["-"], stdin=subprocess.PIPE,
                                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT, bufsize=0)
            self.q = queue.Queue()
            threading.Thread(target=self._pump, daemon=True).start()
            self.ser = self.sock = None
            self.name = f"exec {args.exec_cmd}"
        elif args.tcp:
            host, _, port = args.tcp.partition(":")
            self.sock = socket.create_connection((host, int(port or 23)), timeout=5)
            self.sock.settimeout(0.05)
            self.ser = None
            self.name = f"tcp {host}:{port or 23}"
        else:
            try:
                import serial  # pyserial
            except ImportError:
                sys.exit("pyserial is missing: pip install pyserial")
            self.ser = serial.Serial(args.port, args.baud, timeout=0.05)
            self.sock = None
            self.name = f"serial {args.port} @ {args.baud}"

    def _pump(self):
        while True:
            b = self.proc.stdout.read(1)
            if not b:
                break
            self.q.put(b)

    def write(self, data: bytes):
        if self.proc:
            self.proc.stdin.write(data)
            self.proc.stdin.flush()
        elif self.ser:
            self.ser.write(data)
        else:
            self.sock.sendall(data)

    def _read_some(self) -> bytes:
        if self.proc:
            out = b""
            try:
                out += self.q.get(timeout=0.05)
                while True:
                    out += self.q.get_nowait()
            except queue.Empty:
                pass
            return out
        try:
            if self.ser:
                return self.ser.read(256)
            return self.sock.recv(256)
        except (socket.timeout, BlockingIOError):
            return b""

    def lines(self, timeout: float):
        """Yield received lines until timeout seconds pass without data."""
        deadline = time.time() + timeout
        while time.time() < deadline:
            chunk = self._read_some()
            if chunk:
                self.buf += chunk
                deadline = time.time() + timeout
            while b"\n" in self.buf:
                raw, self.buf = self.buf.split(b"\n", 1)
                line = raw.decode("utf-8", "replace").strip("\r\x00 ")
                if line:
                    if self.verbose:
                        print(f"   <- {line}")
                    yield line

    def drain(self, timeout=0.5):
        return list(self.lines(timeout))

    def command(self, line: str, timeout=2.0):
        """Send one line, collect replies until ok/error (or timeout)."""
        # drop anything left over from the previous command
        for l in self.lines(0.05):
            pass
        if self.verbose:
            print(f"   -> {line}")
        if line == "?":
            self.write(b"?")  # realtime command, no newline (like the pendant)
        else:
            self.write(line.encode() + b"\n")
        replies = []
        for l in self.lines(timeout):
            replies.append(l)
            if line == "?" and l.startswith("<"):
                break
            if l == "ok" or l.startswith("error:"):
                break
        return replies

    def close(self):
        if self.proc:
            self.proc.kill()
        elif self.ser:
            self.ser.close()
        else:
            self.sock.close()


# ---------------------------------------------------------------- scenarios


class Result:
    def __init__(self):
        self.passed = 0
        self.failed = 0

    def check(self, name, ok, detail=""):
        mark = "PASS" if ok else "FAIL"
        print(f"[{mark}] {name}" + (f"  - {detail}" if detail else ""))
        if ok:
            self.passed += 1
        else:
            self.failed += 1


def ended_ok(replies):
    return bool(replies) and replies[-1] == "ok"


def errors(replies):
    return [r for r in replies if r.startswith("error:") or "ERR" in r]


def sc_status(link, res, args):
    r = link.command("?")
    m = next((STATUS_RE.search(x) for x in r if STATUS_RE.search(x)), None)
    res.check("status report (?)", m is not None, m.group(0) + ">" if m else f"got {r}")
    return m.group("state") if m else None


def sc_unlock(link, res, args):
    r = link.command(PENDANT["unlock"])
    res.check("unlock ($X)", ended_ok(r), " | ".join(r))


def sc_home(link, res, args):
    r = link.command(PENDANT["home"], timeout=args.home_timeout)
    res.check("home ($H)", ended_ok(r), " | ".join(r))


def sc_jog(link, res, args):
    for key in ("jog_x+", "jog_x-", "jog_xy", "jog_z+"):
        r = link.command(PENDANT[key])
        res.check(f"jog {PENDANT[key]}", ended_ok(r), " | ".join(errors(r)))
        time.sleep(0.3)
    for key in ("goto_xy0", "goto_z0"):
        r = link.command(PENDANT[key])
        res.check(f"go to zero {PENDANT[key]}", ended_ok(r), " | ".join(errors(r)))
        time.sleep(0.3)
    res.check("machine back to Idle after jogging", wait_idle(link) == "Idle")


def sc_list(link, res, args):
    r = link.command(PENDANT["list"], timeout=3)
    files = [FILE_RE.search(x) for x in r if FILE_RE.search(x)]
    mks_format = any(x.startswith("[FILE:/") for x in r)
    res.check("SD list ([ESP210])", ended_ok(r), f"{len(files)} file(s)")
    res.check("SD list in MKS format ([FILE:/...])", mks_format,
              "only on the pendant UART with mks_pendant:" if not mks_format else "")
    for f in files:
        print(f"         {f.group('name')}  {f.group('size')} bytes")
    return [f.group("name") for f in files]


def sc_frame(link, res, args):
    if not args.file:
        res.check("frame ([ESP801])", False, "use --file NAME")
        return
    r = link.command(PENDANT["frame"].format(file=args.file), timeout=3)
    b = next((BOUNDS_RE.search(x) for x in r if BOUNDS_RE.search(x)), None)
    res.check("frame bounds reported", b is not None, b.group(0) if b else " | ".join(r))
    res.check("frame accepted", ended_ok(r), " | ".join(errors(r)))
    time.sleep(0.5)
    state = wait_idle(link, args.job_timeout)
    res.check("frame finished (Idle)", state == "Idle", f"state {state}")


def wait_idle(link, timeout=60.0):
    deadline = time.time() + timeout
    state = None
    while time.time() < deadline:
        state = sc_status_quiet(link)
        if state in ("Idle", "Alarm"):
            return state
        time.sleep(0.3)
    return state


def sc_status_quiet(link):
    for x in link.command("?"):
        m = STATUS_RE.search(x)
        if m:
            return m.group("state")
    return None


def sc_run(link, res, args):
    if not args.file:
        res.check("run ([ESP220])", False, "use --file NAME")
        return
    # FluidNC answers "ok" to [ESP220] only when the job ENDS, so do not wait for it here.
    for l in link.lines(0.05):
        pass
    link.write(PENDANT["run"].format(file=args.file).encode() + b"\n")
    seen, finish, err, ok_at_end = [], False, [], False
    started = False
    deadline = time.time() + args.job_timeout
    while time.time() < deadline:
        link.write(b"?")
        for l in link.lines(0.4):
            seen.append(l)
            if l.startswith("<Run"):
                started = True
            if l.startswith("error:") or "ERR" in l:
                err.append(l)
            if l == "ok":
                ok_at_end = True
            if "SD Print Finish!" in l:
                finish = True
        if err or (started and ok_at_end and any(x.startswith("<Idle") for x in seen[-3:])):
            break
        time.sleep(0.3)
    res.check("run started ([ESP220])", started and not err, " | ".join(err))
    res.check("job finished, ok received at the end", ok_at_end and not err)
    if not finish:
        for l in link.lines(1.0):
            finish = finish or "SD Print Finish!" in l
    res.check("'SD Print Finish!' received at job end", finish, "" if finish else "only on the pendant UART with mks_pendant:")


def sc_laser(link, res, args):
    r = link.command(PENDANT["laser_test"])
    blocked = any("ignored" in x for x in r)
    res.check("laser test button (M3 S5) is blocked", blocked,
              " | ".join(r) + ("" if blocked else "  <- torch would fire! (needs mks_pendant: allow_spindle: false)"))
    link.command("M5")


def sc_mks_settings(link, res, args):
    for key in ("wifi_mode", "wifi_ssid", "wifi_pass"):
        r = link.command(PENDANT[key])
        res.check(f"MKS setting {PENDANT[key]} accepted", ended_ok(r), " | ".join(errors(r)))


SCENARIOS = {
    "status": sc_status,
    "unlock": sc_unlock,
    "home": sc_home,
    "jog": sc_jog,
    "list": sc_list,
    "frame": sc_frame,
    "run": sc_run,
    "laser": sc_laser,
    "mks": sc_mks_settings,
}
# "all" = everything that does not need a file or homing switches
ALL = ["status", "unlock", "jog", "list", "laser", "mks"]


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    g = p.add_mutually_exclusive_group(required=True)
    g.add_argument("--port", help="serial port (COM5, /dev/ttyUSB0)")
    g.add_argument("--tcp", help="HOST[:PORT] telnet (default port 23)")
    g.add_argument("--exec", dest="exec_cmd", help="FluidNC PC simulator program to start")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--file", help="SD file for frame/run")
    p.add_argument("--home-timeout", type=float, default=60)
    p.add_argument("--job-timeout", type=float, default=300)
    p.add_argument("-v", "--verbose", action="store_true", help="show every line sent/received")
    p.add_argument("steps", nargs="+",
                   help="scenarios: " + ", ".join(SCENARIOS) + ", all  |  send \"<line>\" for a raw command")
    args = p.parse_intermixed_args()

    link = Link(args)
    print(f"Connected: {link.name}")
    for l in link.drain(1.0):
        print(f"   (startup) {l}")

    res = Result()
    steps = list(args.steps)
    i = 0
    while i < len(steps):
        step = steps[i]
        if step == "send":
            i += 1
            line = steps[i] if i < len(steps) else ""
            print(f"-> {line}")
            for l in link.command(line, timeout=3):
                print(f"<- {l}")
        elif step == "all":
            for s in ALL:
                print(f"--- {s}")
                SCENARIOS[s](link, res, args)
        elif step in SCENARIOS:
            print(f"--- {step}")
            SCENARIOS[step](link, res, args)
        else:
            sys.exit(f"unknown step '{step}' (choose from {', '.join(SCENARIOS)}, all, send)")
        i += 1

    link.close()
    if res.passed or res.failed:
        print(f"\n{res.passed} passed, {res.failed} failed")
    sys.exit(1 if res.failed else 0)


if __name__ == "__main__":
    main()
