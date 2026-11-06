# Architecture and implementation boundaries

The current primary UI is real LVGL/WebAssembly. See [lvgl-web.md](lvgl-web.md)
for its build, shared renderer, tests and hardware integration boundaries.

## Implemented and locally executable

- `mock/`: dependency-free Swedish UI, four screens, fictional local fixtures,
  wall-clock playback simulation, seek boundaries, volume, resume preference,
  localStorage recovery and unavailable-storage fallback. Network and audio are simulated.
- `firmware/include/pod/Model.hpp`: podcast, episode, GUID, enclosure, MIME,
  duration, position, completion and subscription models. Episode identity is
  `(podcastId, id)`, never a title.
- `RssFeedParser`: incremental RSS 2.0 subset, channel/item metadata, image,
  enclosure URL/type/length, dates, GUID, iTunes duration and artwork. Handles
  named/numeric entities, UTF-8 and CDATA. Missing enclosures skip unplayable items;
  missing titles receive a fallback; absent GUIDs use a defined FNV-1a hash of the
  audio URL. Duplicate GUIDs within a feed are ignored. URL-based IDs can change
  if a publisher rotates signed URLs; GUIDs remain preferable.
- `FeedRepository`: streams a `NetworkClient` into the parser and only replaces
  the caller's episode list after a successful complete parse.
- `MockAudioPlayer`: explicit test double with play/pause/resume/stop, simulated
  position, completion, bounded seeking, volume and unsupported-seek reporting.
- `Application`: worker-side commands and saved-position restoration, explicit
  results on failed/unsupported operations. It does not create FreeRTOS tasks.
- `StateCodec`, `BlobStateStore`, `ProgressPersistence`: versioned CRC-protected
  records, bounded decoding, subscriptions/last episode/position/volume/resume,
  30-second progress interval and writes on important transitions. Memory and
  test blob stores exercise the interface; no actual NVS backend is supplied.
- `WifiReconnector`: worker-side capped exponential retry schedule, tested with
  an injected fake Wi-Fi adapter. No Wi-Fi radio access or credentials are implemented.
- `PlayerUi`: actual LVGL 8.3.10 widgets, shared C++ screen code and portable font/Lucide assets in `firmware/lvgl/`, compiled and tested through the browser port.
- `Ui` / `LvglPort`: earlier abstract layout plans retained for native tests/reference; no longer the primary renderer.
- `Board`: complete supplied GPIO map, configurable V1/V2 selection and testable
  address-probe policy. Neither controller driver is implemented.

## Memory and parser contract

Maximum input: 512 KiB per feed, 8 KiB per lexical token, 2 KiB per consumed field,
24 nested elements, 40 accepted episodes. Data is consumed incrementally; the full
feed is never retained. Limits return `LimitExceeded`; malformed/unsupported XML
returns `InvalidData`. Callers must discard partial results on any non-OK status.
The parser rejects DTDs and custom/external entities. It is a deliberately small
RSS subset, not a validating general XML implementation. UTF-8 feeds with plain
or CDATA titles are the supported target; HTML nested within titles, alternate
namespace prefixes, Atom, legacy encodings and XML BOM handling are not implemented.
Unknown duration becomes zero (unknown), not a guessed length. Publisher dates
remain raw strings in firmware. No artwork fetch/decode is implemented.

## Hardware boundaries awaiting dependencies

| Boundary | Current behavior | Required integration |
| --- | --- | --- |
| `PlayerUi` | Real LVGL widgets tested in WASM, mock playback | ESP32 display/input driver and real service wiring |
| `St7789DisplayDriver` | `Unavailable` | Verified vendor display implementation |
| `Cst328TouchDriver`, `Cst3530TouchDriver` | `Unavailable` | Selected vendor controller, bus/reset/coordinates |
| `PendingAudioPlayer` | `Unavailable`, seek `Unsupported` | Verified ESP32-audioI2S adapter |
| `NetworkClient` | Interface, fixture implementation in tests | HTTP/HTTPS, CA trust, timeout, redirect policy, bounded streaming |
| `ItunesSearchService` | URL construction implemented; search `Unavailable` | Verified JSON + HTTPS adapter; reject results missing feedUrl |
| `WifiAdapter` | Interface only | Stored credentials, provisioning, state and reconnect |
| `BlobStorage` | Interface and test backend | Atomic Preferences/NVS read/write + error handling |
| Offline audio cache | UI fixture flag only | MicroSD downloader, partial-file rename, quotas, metadata |

The firmware `setup()` is a fail-closed scaffold, not a functional device app.
It does not silently substitute mock playback. Browser demo data is never fetched
from its reserved `example.invalid` URLs.

## Runtime integration design

Use one GUI task for LVGL and a bounded FreeRTOS command queue (e.g. 16 commands)
to an application worker. UI callbacks enqueue IDs/commands without performing
HTTP, XML parsing, flash writes or audio setup. Worker publishes immutable UI
snapshots through a separate queue. The renderer runs only in the GUI context.
Keep audio decoding serviced independently from feed parsing and artwork rendering;
measure task priorities, buffer sizes and stack budgets on the board before fixing
core affinity. Check every queue send; show busy/failure rather than dropping actions.

HTTPS adapters must verify TLS using a trusted CA configuration and enforce request
timeouts, a maximum redirect count, scheme checks and the parser byte budget.
Do not use insecure TLS modes. Credential provisioning and NVS storage are separate
from the public playback record. Never log passwords or commit credentials.

Resume seeks must use a verified decoder capability and return `Unsupported` when
time-based seeking is unavailable. Never derive byte offsets from duration alone.
M4A support is container/codec dependent; test representative files before claiming
it. Network interruption recovery must preserve position and surface buffering/errors.

## Dependency decisions

The first iteration was entirely offline. After explicit authorization, LVGL 8.3.10
and Emscripten 3.1.64 were downloaded for the real browser UI. PlatformIO, Arduino
CLI, ESP32 SDK and ESP32-audioI2S remain unavailable locally.

| Dependency | Intended version | Status / license consideration |
| --- | --- | --- |
| Arduino ESP32 core | 3.x, exact compatible release to be pinned after inspection | Not installed; do not assume compatibility with stock PlatformIO core 2.x |
| LVGL | 8.3.10 | Installed, compiled to WASM and tested; MIT notices retained |
| ESP32-audioI2S | 2.0.0 (user-supplied vendor reference) | Not installed; GPL-3.0 implications must be reviewed before distributing combined firmware |
| Vendor display/touch drivers | Revision-specific, exact version pending | Not installed; inspect APIs and licenses |
| PlatformIO | Exact local version pending | Not installed; platform/framework/toolchains must already be cached for offline builds |

The repository retains its existing license. Dependency license compatibility and
corresponding-source obligations must be resolved before binary distribution.
No unverified vendor or decoder code is vendored into this repository.
