# Real LVGL in the browser

## Run the prepared prototype

The primary prototype is now `web/index.html`. Every on-device element is drawn
by **LVGL 8.3.10 compiled from C/C++ into WebAssembly**, including labels, Lucide
images, buttons, scroll containers, sliders, switch and loading spinner. The DOM
contains a 240 × 320 canvas, not an HTML reconstruction of the device UI.
The original HTML implementation remains in `mock/` as a design reference.

The prepared runtime artifacts are included in `web/dist/` (about 330 KiB combined).
To run without rebuilding:

```sh
cd /workspace/PodPlayer
python3 -m http.server 8766 --bind 127.0.0.1 --directory .
```

Open `http://127.0.0.1:8766/web/` on the same machine. Static HTTP serving is required
for the browser's WASM fetch; do not open this version as `file://`. All requests
stay on the local origin. There are no CDNs, package installs, remote audio or
online font requests at runtime. Stop the server with Ctrl+C.

Drag episode rows vertically to scroll, then tap a row to open the paused player.
Play/pause, ±10 seconds, the position slider and settings are LVGL controls. The
outer HTML controls only set preview size, select development scenarios and reset
the simulation. Playback, Wi-Fi and saved-audio availability remain explicit mocks.

## Build again using the installed tools — offline

```sh
cd /workspace/PodPlayer
python3 scripts/build-web.py
```

Defaults: LVGL sources at `/workspace/deps/lvgl-8.3.10`, Emscripten at
`/workspace/deps/emsdk`. Override `LVGL_DIR` / `EMSDK`, or `EMCC` / `EMXX` when needed.
The script requires the exact LVGL 8.3.10 version, compiles 194 source files with
up to four workers, reuses valid object files, and writes JS + WASM + the LVGL
license into `web/dist/`. Changes in UI headers/configuration invalidate the cache.
Sources must remain immutable at the pinned LVGL revision; after intentionally
changing dependency headers or switching compiler configuration, remove
`build/lvgl-web/` to force a clean build. No network access occurs in this script.
The first Emscripten build may populate its local system-library cache.

## Recreate dependencies — networked, explicitly authorized setup

```sh
cd /workspace/PodPlayer
bash scripts/setup-web.sh
```

This downloads the official repositories and Emscripten SDK, without system-wide
package installation. `PODPLAYER_DEPS_DIR` defaults to `/workspace/deps`. Existing
paths and local changes are preserved; a mismatched dependency fails with an error.
Uses normal verified HTTPS and the platform Git authentication/proxy. Tasks already
run in isolated environments; use the existing checkout and do not create a Git
worktree unless explicitly requested.

Pinned inputs:

| Input | Version / identity |
| --- | --- |
| LVGL | 8.3.10, commit `06e229cc7679c01119ca5d905a1f9dad96f475f0` |
| Emscripten | 3.1.64, releases build `fd61bacaf40131f74987e649a135f1dd559aff60` |
| emsdk scripts | Commit `d7b4130688bfa348392b921ab04015aa15c4f154` |
| Lucide artwork | 1.8.0, existing local SVG subset, rasterized into LVGL alpha masks |
| Font | Locally installed DejaVu Sans, generated 8-bit glyph masks including Swedish Latin-1 characters |

Network destinations needed for recreating these dependencies include `github.com`
and `storage.googleapis.com` (the SDK's official artifact host and redirect hosts).
A Debian-package alternative was downloaded while SDK access was blocked, but is
**not used by the build**. The successful build uses the official SDK above.

Generated fonts/icons are committed C++ arrays; builds do not need Pillow, Sharp,
Node packages or source fonts. Optional regeneration commands:

```sh
python3 scripts/generate-lvgl-fonts.py # needs existing Pillow and local DejaVu fonts
SHARP_MODULE=/absolute/path/to/existing/sharp node scripts/generate-lvgl-icons.cjs
```

The build regenerates `DemoFixtures.hpp` from `fixtures/*.json` with Python's standard
library. License notices are retained in `firmware/lvgl/assets/` and `web/dist/`.

## Code ownership and portability

- `firmware/lvgl/PlayerUi.cpp`: shared LVGL screen tree, styles, events, demo session
  state and simulated playback. It has no Emscripten, canvas or DOM dependencies.
  All calls run on a single GUI context. Event callbacks mark the view dirty;
  tree deletion/rebuild is deferred to the next frame so callback user data stays valid.
- `firmware/lvgl/assets/`: portable C++ font/image resources. Lucide SVGs become
  LVGL `ALPHA_8BIT` images with recoloring; no runtime SVG decoder is needed.
- `web/main.cpp`: Emscripten display/input driver, RGB565 canvas flush, LVGL tick
  and requestAnimationFrame loop. Pointer transitions are queued so rapid clicks
  between LVGL input polls are preserved; pointer cancellation/blur releases input.
- `web/host.js`: pixel presentation, pointer coordinate mapping, localStorage bridge,
  boot/error status and external developer controls. No device UI widgets or
  playback logic are implemented in JavaScript.
- `firmware/src/Ui.cpp` / `LvglPort`: earlier abstract layout plans retained for
  existing native tests/reference; the active renderer is now `PlayerUi.cpp`.

To reuse on ESP32, compile the shared `PlayerUi.cpp` and asset sources with LVGL
8.3.10, initialize an actual display and touch port, and call `begin()` and `tick()`
from the GUI task. Replace the demo session/backend wiring with the real application
worker and audio service. The shared source currently explicitly owns a mock audio
player; this is not yet real decoder integration or a new flashable firmware entry
point. Arduino `setup()` remains the earlier unavailable-driver scaffold.

Browser persistence is versioned, uses stable episode IDs, clamps restored positions
and volume, never auto-starts playback, and tolerates unavailable localStorage.
Progress writes occur every 30 seconds while playing and on important transitions
and page exit. Positions for other episodes are retained. The browser store is
separate from the future hardware Preferences/NVS adapter.

## Automated validation

With the static server running:

```sh
PLAYWRIGHT_MODULE=/opt/codex/cua_node/lib/node_modules/playwright \
  LVGL_BASE_URL=http://127.0.0.1:8766/web/ node tests/lvgl-browser.cjs
bash scripts/test.sh
```

Playwright/Chromium must already be installed; they are development-only tools.
The browser test clicks real canvas coordinates, observes read-only C++ state,
requires a successful `.wasm` response, and rejects external resource requests.
It exercises navigation, rapid clicks, play/pause/timing, ±10, seek boundaries,
volume, resume preference, persisted reload, all five scenarios, scrolling, denied
storage, native/enlarged preview and mobile reflow. Screenshots are saved under
ignored `test-results/lvgl-*.png`.

Canvas widgets do not expose a DOM accessibility tree. This prototype is suitable
for touch/visual iteration, not a claim of browser screen-reader accessibility.
Physical touch, display timing, PSRAM, power use, Wi-Fi, I²S/DAC, SD and decoder
behavior remain untested until the actual board is connected.
