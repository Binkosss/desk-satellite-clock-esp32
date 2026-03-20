# Changelog

All notable changes to this project are documented in this file.

## [1.0.3] - 2026-03-20
### Added
- Configurable LED activity window in `src/main.cpp` via:
	- `LED_ACTIVE_HOUR_START`
	- `LED_ACTIVE_HOUR_END`

### Changed
- LED outputs are now forced OFF outside active hours.
- Default schedule keeps LEDs OFF from 21:00 to 06:59 (local time).

## [1.0.2] - 2026-03-14
### Added
- Repository structure for hardware documentation:
	- `docs/photos/` for project photos
	- `hardware/pcb/fusion360/` for Fusion Electronics source files (`.fsch`, `.fbrd`)
	- `hardware/pcb/gerbers/` and `hardware/pcb/exports/` for fabrication and documentation outputs

### Changed
- `README.md` updated with photo directory and Fusion PCB file location details.

## [1.0.1] - 2026-03-14
### Added
- Global timezone configuration via `TIMEZONE_POSIX` in `include/secrets.h`.
- Example timezone settings in `include/secrets.h.example`.
- Wi-Fi auto-reconnect in display task (retries every 10 s on connection loss).

### Changed
- Time handling now uses POSIX timezone rules for automatic standard time / DST switching.
- Date display format changed from `YY-MM-DD` to `DD-MM-YY`.
- Device always starts in time display mode after reset or power loss.
- NTP sync throttled to once per second to reduce overhead.
- LED animation task now uses `vTaskDelay` to yield CPU and fix long-uptime slowdown.
- README updated with timezone setup instructions and examples.

## [1.0.0] - 2026-03-14
### Added
- Public release documentation: `README.md`, `CONTRIBUTING.md`, `LICENSE`.
- Credentials template: `include/secrets.h.example`.
- Source header in `src/main.cpp` with author, date, and purpose.

### Changed
- Refactored variable names for readability and consistency.
- Replaced hardcoded Wi-Fi credentials with `secrets.h`-based local config.

## [0.6.0]
### Added
- FreeRTOS split into two pinned tasks:
	- display/task logic
	- LED/NeoPixel animation logic
- Integrated full feature set in one program (NTP, MAX7219, DHT11, button, LEDs).

### Changed
- Migrated from single-loop orchestration to task-based runtime model.

## [0.5.0]
### Added
- NeoPixel smooth color transition prototype.
- Single LED blink pattern (short blinks with pause cycle).

## [0.4.0]
### Added
- Button-driven mode switching with debounce handling.
- Display modes for time/date/temperature/humidity.

### Changed
- Replaced timer-only screen switching with manual state control.

## [0.3.0]
### Added
- DHT11 sensor integration into display flow.
- Periodic temperature and humidity reads with NaN safety checks.

## [0.2.0]
### Added
- Wi-Fi + NTP synchronization.
- Time/date rendering on MAX7219 with fixed format (`HH-MM-SS`, `YY-MM-DD`).

## [0.1.0]
### Added
- Hardware bring-up and module-level prototypes:
	- I2C scan utility
	- MAX7219 text/character test sketches

