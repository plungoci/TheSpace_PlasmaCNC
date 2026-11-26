# TheSpace WebUI

A dark, plasma-oriented web interface for FluidNC, served by the controller itself
(no internet needed on the workshop WiFi).

- Live top view of the cutting bed: torch crosshair, work origin, frame of the selected job,
  cut trail (orange while the torch is on)
- Large work + machine coordinate readouts, per-axis zero, go to X0 Y0
- Jog pad (XY + diagonals, Z), step and feed presets, arrow keys / PgUp / PgDn, Esc = STOP
- STOP (feed hold + reset), Unlock, Reset always in the top bar
- SD card: list, frame (`$MKS/Frame`, torch off), run, delete, upload (drag & drop)
- Job bar with progress, pause, resume, stop
- Plasma: hold-to-fire torch test (armed, auto-off), IHS test, input LEDs (probe, limits, door)
- Console with history and quick commands
- Opened from a PC file or a preview it runs a simulated machine (demo mode)

## Install on the board

1. Open the current WebUI (http://192.168.0.1 on the `FluidNC` WiFi).
2. In the file manager for the local filesystem (FluidNC flash, not SD), upload
   `dist/webui3.html.gz` first - this keeps the original FluidNC WebUI at `/webui3.html`.
3. Upload `dist/index.html.gz` (replaces the original start page).
4. Reload http://192.168.0.1

## Build

`dist/index.html.gz` is `index.html` compressed: `gzip -9 -n -c webui/index.html > webui/dist/index.html.gz`

## Protocol used
- WebSocket `ws://<board>/` - commands as text lines, realtime `?` `!` `~` Ctrl-X and jog cancel 0x85;
  controller output arrives as binary frames
- `GET /upload?path=/` SD list (JSON), `&action=delete&filename=` delete, `POST /upload` multipart upload
