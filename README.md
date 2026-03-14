# Desk Satellite Clock (ESP32)

A PlatformIO/Arduino project for a custom desk clock shaped like a small satellite.
The device runs on an ESP32 and a custom PCB milled on a CNC 3018.

It provides:
- Time and date from NTP on MAX7219 display
- Temperature and humidity from DHT11
- Two NeoPixels at the bottom (rocket-engine style effect)
- One top LED "antenna" blink (satellite beacon effect)
- Vintage clock-style 7-segment LED module with lens look (9 digits total, last digit intentionally unused)

## Author
- **Binki**

## Project Goal
Build a reliable, visually distinctive desk clock that combines practical data (time/date/environment) with a satellite-inspired LED identity.

## Design Story
This project was designed as a desk clock shaped like a small satellite.
The PCB is custom-made and milled on a CNC 3018, blending DIY hardware with practical daily use.
Two bottom NeoPixels simulate engine glow, while a single top LED acts as a satellite beacon.

### Alternative A (Technical)
Desk Satellite Clock is an ESP32-based desktop device built on a custom CNC 3018 milled PCB.
It combines NTP time/date on a MAX7219-driven vintage 9-digit 7-segment module (last digit intentionally unused),
DHT11 environmental sensing, and FreeRTOS-separated display/animation tasks.

### Alternative B (Showcase)
A handmade satellite-inspired desk clock with its own custom PCB, milled on a CNC 3018.
Twin NeoPixels at the base imitate engine glow, and a top beacon LED blinks like an antenna in orbit,
while the front display shows live time, date, and room conditions.

## Features
- Dual FreeRTOS tasks (display + LED animation)
- Button-based mode switching (time/date/temp+humidity)
- Smooth NeoPixel color transition
- NTP sync over Wi-Fi

## Repository Structure
- `src/main.cpp` – main application code
- `include/secrets.h.example` – Wi-Fi credentials template
- `docs/archive/` – archived experimental notes/copies
- `lib/` – local third-party libraries
- `CHANGELOG.md` – release and change history

## Quick Start
### 1) Configure Wi-Fi secrets
1. Copy `include/secrets.h.example` to `include/secrets.h`
2. Set your credentials:

```cpp
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
```

`include/secrets.h` is ignored by Git.

### 2) Build
```bash
pio run
```

### 3) Upload
```bash
pio run -t upload
```

### 4) Serial monitor
```bash
pio device monitor -b 115200
```

## Hardware (used in code)
- ESP32 DevKit V1
- Vintage lens-style 9-digit 7-segment LED display module (MAX7219-driven; last digit intentionally unused)
- DHT11 on GPIO 15
- Button on GPIO 13 (`INPUT_PULLUP`)
- Single LED on GPIO 18
- NeoPixel data on GPIO 5
- MAX7219 display module

## Notes
- Time offset is currently configured for CET/CEST values in code.
- For public sharing, do not commit `include/secrets.h`.

## License
MIT — see `LICENSE`.
