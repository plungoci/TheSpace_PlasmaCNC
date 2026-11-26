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
- Settings: every FluidNC setting, editable - the whole config.yaml tree (`$CD`, applied with
  `$/path=value`) and the stored controller settings (`$S`, which include the `$$` Grbl ones as
  `$Grbl/...`), with search, Save to config.yaml (`$CD=config.yaml`) and Restart (`$Bye`)
- Setup: **axis calibration** (move a set distance, enter the measured one, new steps/mm is
  computed, applied and saved) with a theoretical steps/mm calculator (belt, lead screw, rack);
  IHS values, bed size, and **Board files** - list, download, delete and upload files on the
  controller flash (config.yaml, this page). Upload a new `index.html.gz` there to update the UI.
- Opened from a PC file or a preview it runs a simulated machine (demo mode)

## Install on the board

1. Connect to the `FluidNC` WiFi and open FluidNC's built-in file manager:
   **http://192.168.0.1/?forcefallback=yes** (always available, whatever index.html is on the board).
2. Upload `dist/index.html.gz` - it replaces the start page.
3. Reload http://192.168.0.1 with Ctrl+F5.

FluidNC serves only `index.html(.gz)` as a page; any other file on the flash (for example a
renamed copy of the original WebUI) is offered as a download, not displayed. The original
FluidNC WebUI is kept in `dist/webui3.html.gz` - to go back, upload it **as** `index.html.gz`.

## Update the UI later

Setup tab -> Board files -> upload the new `index.html.gz` (it overwrites the old one), then reload
with Ctrl+F5. Without this page: http://192.168.0.1/?forcefallback=yes, or from a PC on the board's WiFi:

```bash
curl -F "path=/" -F "myfile[]=@index.html.gz;filename=/index.html.gz" http://192.168.0.1/files
```

## Build

`dist/index.html.gz` is `index.html` compressed: `gzip -9 -n -c webui/index.html > webui/dist/index.html.gz`

## Protocol used
- WebSocket `ws://<board>/` - commands as text lines, realtime `?` `!` `~` Ctrl-X and jog cancel 0x85;
  controller output arrives as binary frames
- `GET /upload?path=/` SD list (JSON), `&action=delete&filename=` delete, `POST /upload` multipart upload
- `GET /files?path=/`, `POST /files` - the same for the controller flash (LocalFS)
- `GET /command?plain=<cmd>` - synchronous command, reply text in the body (settings tab)
