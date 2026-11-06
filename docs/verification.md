# Verification record

Initial validation was performed in the existing cloud checkout using local tools
and fixtures, without package or documentation downloads. The later authorized
LVGL/WebAssembly iteration is recorded separately below.

| Check | Result |
| --- | --- |
| `bash scripts/test.sh` | Passed: 353 native assertions, 5 JavaScript tests |
| Native build with `-DPODPLAYER_TOUCH_VERSION=1` | Passed: same 353 assertions with V1 fallback selection |
| Native AddressSanitizer + UndefinedBehaviorSanitizer build/run | Passed: 353 assertions; execution required leaving the command sandbox because LeakSanitizer cannot run under its ptrace restriction |
| `tests/browser.cjs`, installed Playwright and Chromium | Passed over localhost: navigation, play/pause/timer, ±10 seek, volume, persistence/reload, loading/empty/offline/error/format states, unavailable storage, native/enlarged sizing and mobile reflow |
| Browser external resource requests | Zero |
| Browser visual inspection | Library, player, settings and 390 px mobile layout inspected; clipped artwork text corrected; redundant settings action removed |
| `git diff --check` | Passed |
| ESP32 firmware build | Not run: PlatformIO, framework and toolchain unavailable |
| Physical hardware | Not tested |
| Direct `file://` Chromium navigation | Blocked by managed-browser URL policy; static localhost assets used instead. Direct local-file use is implemented but not validated in this managed browser |

Native sanitizer reproduction:

```sh
mkdir -p build
g++ -std=c++17 -fsanitize=address,undefined -fno-omit-frame-pointer -g \
  -Ifirmware/include firmware/src/RssFeedParser.cpp firmware/src/StateCodec.cpp \
  firmware/src/Ui.cpp firmware/test/test_main.cpp -o build/native-tests-sanitized
./build/native-tests-sanitized fixtures/sample-podcast.xml
```

V1 reproduction: use the native compilation command from `scripts/test.sh`, add
`-DPODPLAYER_TOUCH_VERSION=1`, and run the resulting executable with the same RSS
fixture argument. All tests are fixture-driven; none establishes live RSS, iTunes,
decoder, display, touchscreen, NVS or Wi-Fi functionality.

## LVGL/WebAssembly iteration

After explicit authorization to download build dependencies, the official LVGL
8.3.10 and Emscripten 3.1.64 SDK were installed under `/workspace/deps`.
`python3 scripts/build-web.py` compiled 194 C/C++ sources and linked actual WASM.
`tests/lvgl-browser.cjs` passed with real LVGL canvas interactions, rapid clicks,
play/pause/timing, seeking/seek boundaries, volume, resume preference, saved reload,
all five scenarios, scrolling, preview scaling, mobile reflow and unavailable storage.
No external runtime requests occurred. Native tests still passed: 353 assertions
and five JavaScript tests. Browser screenshots were inspected for library, episode,
player and settings views. No hardware test or ESP32 compilation was performed.
