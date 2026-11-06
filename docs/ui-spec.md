# PodPlayer UI specification — version 0.1

The primary implementation is now the real LVGL/WebAssembly app in `web/`;
`mock/` retains the earlier HTML design reference. See [lvgl-web.md](lvgl-web.md).
The browser prototype is the visual reference. All dimensions below are native
240 × 320 screen pixels, independent of the outer device-frame preview scale.
Swedish UI text; all podcasts and episode titles are fictional demo data.

## Shared foundations

- Screen: portrait, fixed 240 × 320; 20 px status strip and 44 px navigation bar.
- Horizontal content inset: 12 px; small gaps 4–8 px, related content gap 10 px.
- Surface `#f6f7f2`, ink `#213132`, secondary text `#627260`, accent `#12685b`.
- Thin dividers `#dce2dc`; muted footer `#e8eee4`; offline banner warm amber.
- The HTML reference uses system sans-serif. The LVGL implementation uses bundled
  DejaVu Sans glyphs including Swedish characters; no online font requests. Headings 19 px/22 px;
  compact navigation 15 px; episode titles 14 px/18 px; lists 12 px/16 px;
  metadata 9–10 px. Native-size hardware readability remains a physical test.
- Controls have at least 44 × 44 px touch areas; main play/pause 56 × 56 px.
  Range inputs have 44 px touch height even though their visual tracks are thin.
- Corners: 8–10 px on artwork/buttons; circular transport controls. Inline local
  Lucide 1.8.0 SVG icons with 2 px strokes, text labels/accessible names for all
  interactive icon controls. Selected original SVG nodes are bundled in
  `mock/vendor/lucide/icons.js`; license notices are retained alongside them.
  The ±10 controls combine Lucide RotateCcw/RotateCw with a separate “10” label.
- Keyboard focus uses a visible warm outline. Reduced-motion preferences disable
  loading animation. No claims of a full accessibility audit.

## A. My podcasts

Heading “Mina poddar”, settings action top right, small “DITT BIBLIOTEK” label.
Three 65 px rows contain 42 px generated artwork, title, episode count, new-count
badge and chevron. Scroll content when more podcasts are added. A 44 px mini-player
at the bottom opens the selected episode; its separate play button toggles playback.
Offline status adds a compact banner without disabling access to local content.

## B. Episodes

Back button returns to podcasts. Podcast title truncates in the navigation bar.
The central list scrolls vertically; rows are at least 64 px and expand for long
titles/partial-progress metadata. Show title, Swedish date and duration. A thin
progress indicator and remaining time distinguish partially played episodes;
completed entries say “Lyssnat”. Selecting a row opens the paused player. Playback
does not start unexpectedly. The mini-player remains reachable at the bottom.

## C. Now playing

Back returns to the selected podcast's episode list. A 68 px artwork tile anchors
the top content beside podcast name and a title of up to three lines. The player
then shows playback status, progress slider, elapsed and remaining time, and a
horizontal transport group: back 10, primary play/pause, forward 10. Volume and the
simulation label appear below. Settings remains available in the navigation bar.

Prototype playback advances using a monotonic clock, pauses immediately and clamps
seeking to `[0, duration]`. Completion pauses; pressing play at the end restarts at
zero. Slider seeking updates the same position. The demo does not request audio.
The firmware must expose actual decoder seeking limitations instead of simulating
success. Full progress repaint avoids replacing the active slider DOM on each tick.

## D. Settings

Back returns to the originating screen. Volume slider shows a percentage, Wi-Fi
row toggles simulated connection, “Kom ihåg position” controls resume, and a compact
device-information block identifies the prototype. The settings body scrolls on
the small screen. Firmware Wi-Fi provisioning is a separate future flow.

## Navigation and state

Podcasts → episodes → player; back reverses the path. Settings is reachable from
the three content screens and returns to its origin. The mini-player opens the
current episode even if another podcast's list is visible. It does not replace the
currently playing source merely by opening another podcast.

State is stored under `podplayer.demo.v1`: selected episode ID, per-episode position,
volume, resume preference and simulated Wi-Fi. On reload playback is paused.
Progress writes occur every 15 seconds while playing and on user transitions/page
exit. localStorage failure displays a notice outside the device and leaves the UI
usable for the current session. Reset clears all demo choices.

## Development scenarios

The scenario selector lives outside the emulated device, never in the device UI.

| Scenario | Visible behavior / recovery |
| --- | --- |
| Loading | Spinner, “Hämtar avsnitt”; explicit demo button completes loading |
| Empty | “Inga avsnitt ännu”, explanation and retry |
| Offline | Cached list remains visible; one fictional saved episode can simulate playback; uncached episodes show connection message and settings action |
| Playback error | “Ljudet kunde inte spelas”, saved-position reassurance and retry |
| Unsupported format | “Ljudformatet stöds inte” and action to choose another episode |

Changing a scenario pauses playback. Retry returns to normal demo state; it does
not pretend to perform a real HTTP operation. Wi-Fi and download indicators are
clearly simulated. “Normal användning” returns to the library.

## Reusable components and firmware mapping

Status strip, navigation bar, icon button, podcast tile, episode row, artwork,
mini-player, progress control, transport group, settings row, switch and message
panel form the component set. The active LVGL widget implementation is
`firmware/lvgl/PlayerUi.cpp`, with portable DejaVu fonts and rasterized Lucide masks.
The earlier `firmware/src/Ui.cpp` abstract plans remain as test/reference material.
The LVGL port uses native 240 × 320 RGB565 rendering; JavaScript presents its pixels.
LVGL episode rows are 76 px to accommodate two-line titles and progress metadata;
preview font rasterization and memory settings are described in `docs/lvgl-web.md`.
Browser persistence for this implementation uses `podplayer.lvgl.v1`, stable episode
IDs and a 30-second progress interval. The earlier HTML store remains separate.
