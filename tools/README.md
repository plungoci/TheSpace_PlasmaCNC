# Tools

## pendant_sim.py - MKS pendant simulator

Sends the same commands as the MKS handheld pendant and checks the answers.

```bash
pip install pyserial            # only for --port

# Real board, through a USB-UART adapter on the pendant connector (J12/J14):
#   adapter TX -> pin 4 (RXD1), adapter RX -> pin 3 (TXD1), GND -> pin 2. Do not connect +5V.
python tools/pendant_sim.py --port COM5 all
python tools/pendant_sim.py --port COM5 frame --file part.nc
python tools/pendant_sim.py --port COM5 run --file part.nc

# Board over WiFi (telnet) - command level only, no pendant translation:
python tools/pendant_sim.py --tcp 192.168.1.50 status jog list

# FluidNC PC simulator (see docs/building.md):
python tools/pendant_sim.py --exec /path/to/program all frame --file part.nc

# One raw line:
python tools/pendant_sim.py --port COM5 send "$MKS/SDList"
```

Scenarios: `status`, `unlock`, `home`, `jog`, `list`, `frame`, `run`, `laser`, `mks`,
`all` (= status unlock jog list laser mks). Add `-v` to see every line.

On the pendant UART (with `mks_pendant:` in config.yaml) every check should pass.
Over USB/telnet/simulator, the pendant-only checks (MKS list format, blocked M3,
ignored $50/$53/$54, `SD Print Finish!`) are expected to fail - those translations
happen only on the pendant channel.

## sim/ - files for the PC simulator

`sim/native_localfs/config.yaml` and `sim/native_sd/part.nc`: run the simulator from the
`tools/sim` folder.
