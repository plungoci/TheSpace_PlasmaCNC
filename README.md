# TheSpace PlasmaCNC

Open-source firmware, configuration and documentation for a DIY CNC plasma cutter
built on the **Makerbase MKS LS ESP32 PRO V2.1** controller (ESP32-S3).

The firmware is based on [FluidNC](https://github.com/bdring/FluidNC) (v4.x, ESP32-S3 support)
with plasma-specific additions kept in separate modules, so upstream FluidNC
updates can still be merged.

> Status: **planning / pre-hardware**. Nothing here has been tested on real hardware yet.

## Goals

- Standalone (no PC) plasma cutting from SD card
- Keep the original **MKS handheld pendant** (RJ cable) working with FluidNC
- **IHS** (Initial Height Sensing) with an inductive sensor / floating head
- **Arc OK** monitoring - stop the job if the arc fails
- Single-wire UART for the on-board TMC2209 X/Y drivers (current, StealthChop/SpreadCycle, sensorless homing)
- Later: **THC** (Torch Height Control) based on arc voltage

## Repository layout

```
configs/      FluidNC config.yaml files per board
docs/
  hardware/   board pinout, connectors, wiring
  plasma/     IHS, Arc OK, THC notes
  mks-pendant.md   protocol of the MKS handheld pendant
firmware/     FluidNC (git subtree, v4.1.1) + plasma modules
tools/        test scripts, pendant simulator (coming)
```

## Building

See [docs/building.md](docs/building.md).

## Roadmap

See [docs/roadmap.md](docs/roadmap.md).

## Safety

Plasma cutting involves high voltage, high-frequency arc start, UV radiation, fumes and fire risk.
This is a hobby project, provided **without any warranty**. You are responsible for your machine.

## License

GPL-3.0, same as FluidNC. See [LICENSE](LICENSE).
