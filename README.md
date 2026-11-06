# PodPlayer

A podcast-player prototype for the Waveshare
ESP32-S3-Touch-LCD-2.8 (SKU 27690). The browser prototype works now. The firmware
contains native-tested domain logic and hardware integration scaffolding; it is
**not yet a playable or flash-validated ESP32 application**.

## Primary prototype: real LVGL / WebAssembly

The current prototype uses **LVGL 8.3.10 in C++**, compiled to WebAssembly.
All on-device widgets are real LVGL; HTML only hosts the canvas and development controls.
Prepared JS/WASM artifacts are included. Start the local server:

```sh
cd /workspace/PodPlayer
python3 -m http.server 8766 --bind 127.0.0.1 --directory .
```

Open `http://127.0.0.1:8766/web/` on that machine. No external runtime requests are
needed. Run `python3 scripts/build-web.py` to rebuild using the installed SDK.
See [LVGL browser instructions](docs/lvgl-web.md) for pinned dependencies, setup,
shared firmware UI source, tests and remaining hardware boundaries.

## Earlier HTML design reference — no installation

Open `mock/index.html` in a browser. All HTML, CSS, JavaScript, icons and fictional
podcast data are local. No framework, npm install, fonts, audio files or network
services are needed. Toggle native size / 150% preview outside the device frame.
Icons use a local subset of Lucide 1.8.0; ISC and inherited Feather MIT notices
are included in `mock/vendor/lucide/LICENSE`. No Lucide runtime is required.

Use the podcast list → episode list → player. Try play/pause, ±10 seconds, the
position slider and settings. The scenario selector exposes loading, empty,
offline, playback-error and unsupported-format states. One episode is marked as
saved to illustrate offline behavior; **no real audio has been downloaded**.

If your managed browser blocks `file://`, serve the same files locally:

```sh
cd /workspace/PodPlayer
python3 -m http.server 8765 --bind 127.0.0.1 --directory mock
```

Then open `http://127.0.0.1:8765` on that machine. This serves static local assets
only. The prototype still makes no external requests. For a different checkout
location, substitute its root directory. localStorage availability on `file://`
depends on the browser; the UI explicitly reports unavailable persistence.

## Offline tests

Requirements already available in the development environment: C++17 `g++`, Bash
and Node.js. No dependency installation is required.

```sh
cd /workspace/PodPlayer
bash scripts/test.sh
```

This compiles/runs native RSS, playback, persistence, UI-layout and touch-selection
checks and runs five JavaScript state tests. Native tests use only standard C++
and local fixtures; they do not compile ESP32/LVGL code or emulate physical drivers.

Optional browser tests require **already installed** Playwright and Chromium:

```sh
mkdir -p test-results
# Run the local static server in another terminal if file:// is blocked.
PLAYWRIGHT_MODULE=/opt/codex/cua_node/lib/node_modules/playwright \
  MOCK_BASE_URL=http://127.0.0.1:8765/index.html \
  node tests/browser.cjs
```

Omit `MOCK_BASE_URL` to test opening the local file on a browser that permits it.
Set `PLAYWRIGHT_MODULE` to your existing module location (or omit when resolvable),
and `CHROMIUM_PATH` if Chromium is not `/usr/bin/chromium`. Test screenshots go to
ignored `test-results/`. Chromium needs local process/IPC permissions; no external
network access is needed. Browser test dependencies are not application dependencies.

## Firmware build configuration

`firmware/platformio.ini` defines the ESP32-S3 target, 16 MB flash, OPI PSRAM,
Arduino framework and C++17. V2/CST3530 (`0x58`) is the default. V1/CST328 (`0x1A`)
is available as a separate environment. An injectable startup selection routine
can probe both addresses; actual I²C calls await a verified hardware adapter.

PlatformIO, Arduino core and board/audio vendor libraries are **not installed here**.
LVGL 8.3.10 and Emscripten 3.1.64 are installed for the browser build. No ESP32
build or flash was attempted, and the exact toolchain combination is not verified.
Before attempting an offline build, manually provide a complete local PlatformIO
installation, an Espressif32 platform compatible with Arduino core 3.x, all cached
framework/compiler packages, and inspected compatible display/touch/LVGL/audio
libraries. The user-supplied vendor reference is LVGL 8.3.10 and ESP32-audioI2S 2.0.0;
the LVGL widget APIs are now compiled and tested in the browser. Compatibility
with the Arduino/audio stack remains unverified. ESP32-audioI2S is
GPL-3.0: review distribution obligations before linking/distributing firmware.

Once those prerequisites are supplied, these are the intended commands, **not
commands verified in this environment**:

```sh
export PODPLAYER_ESP32_PLATFORM_DIR=/absolute/path/to/local/espressif32-platform
export PODPLAYER_LIBRARIES_DIR=/absolute/path/to/inspected/arduino-libraries
cd /workspace/PodPlayer/firmware
pio run -e waveshare_s3_v2
# For V1: pio run -e waveshare_s3_v1
# After integrating hardware adapters and validating the build:
pio run -e waveshare_s3_v2 -t upload --upload-port /dev/ttyACM0
pio device monitor -b 115200 --port /dev/ttyACM0
```

Adjust the serial port to the actual device. Do not flash this scaffold expecting
a working player: `setup()` currently reports an unavailable display adapter to
the debugger and no hardware is started. No library-manager downloads are declared
in the configuration; missing platform transitive packages can still cause
PlatformIO to request network access, so supply the complete local package cache.

## Source map

| Location | Purpose |
| --- | --- |
| `web/` | Primary LVGL/WebAssembly browser port and prepared artifacts |
| `firmware/lvgl/` | Shared LVGL widgets, demo controller, fonts and Lucide assets |
| `mock/` | Earlier HTML design reference, simulated playback |
| `fixtures/` | Fictional podcasts/episodes and RSS fixture |
| `firmware/include/pod/` | Models, interfaces, board map, state machine/services |
| `firmware/src/` | Bounded streaming RSS parser, persistence codec, UI screen plans, Arduino entry scaffold |
| `firmware/test/` | Native tests using mocks and local fixtures |
| `tests/` | JavaScript state tests and optional Chromium interaction tests |
| `docs/ui-spec.md` | Visual/interaction contract for browser and eventual LVGL implementation |
| `docs/architecture.md` | Implemented capabilities, memory limits, integration boundaries and licenses |
| `docs/hardware.md` | Pin map, revision selection and bring-up checklist |

## Remaining work

ESP32 display/touch drivers and LVGL port wiring, audio decoding/I²S, Wi-Fi/TLS,
live iTunes JSON, Preferences/NVS backend, MicroSD caching/downloads and worker/GUI
task wiring await compatible local dependencies and hardware verification.
The shared LVGL widgets are implemented and tested in the browser. The
interfaces return explicit unavailable/unsupported results rather than fake
success. Future device networking is supported by the design; this development
iteration uses offline fixtures exclusively. Never commit Wi-Fi credentials.
