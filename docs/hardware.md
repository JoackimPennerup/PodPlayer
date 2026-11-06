# Hardware integration status

Target: Waveshare ESP32-S3-Touch-LCD-2.8, SKU 27690, ASIN B0D8KKRV5V.
The following specifications were supplied by the user, not measured on a board.

| Component | Configuration |
| --- | --- |
| MCU | ESP32-S3, dual-core Xtensa LX7, up to 240 MHz |
| Memory | 16 MB flash, 8 MB PSRAM |
| Display | ST7789 SPI, 240 × 320 portrait, 2.8-inch IPS |
| Display pins | MOSI 45, SCLK 40, CS 42, DC 41, RST 39, backlight 5 |
| Touch bus | SDA 1, SCL 3, INT 4, RST 2 |
| Touch V1 | CST328, I²C address `0x1A` |
| Touch V2 | CST3530, I²C address `0x58` — default |
| Audio | PCM5101 I²S DAC, onboard amplifier, 8 Ω / 2 W speaker |
| I²S pins | BCLK 48, LRCK 38, DOUT 47 |
| MicroSD SPI | MISO 16, MOSI 17, SCLK 14, CS 21 |

All pins above are reserved for these functions. PCM5101 does not decode MP3/AAC;
the ESP32 must decode compressed audio in software.

## Touch selection

`pod/Board.hpp` defaults `PODPLAYER_TOUCH_VERSION` to `2`. Define it as `1` for V1.
`selectTouch(probe, true)` probes both known addresses through a supplied callback:

- One ACK: select that revision and return `detected=true`.
- Both ACK: return `InvalidData` and no selected revision; do not guess.
- No ACK: return the configured default with `detected=false`. This does **not**
  establish that a touch controller works; the caller must report the fallback.
- `autodetect=false`: use the compile-time selection without probing.

An ACK is address evidence, not chip identity. Actual controller initialization,
reset timing, identification registers and coordinate decoding require the vendor
driver and the physical board. The two adapter types currently return `Unavailable`.
No real I²C bus calls or startup autodetection are wired into `setup()` yet.

## Bring-up sequence when tools and hardware arrive

1. Confirm PCB revision and installed touch controller; inspect vendor source and licenses.
2. Install a compatible local Arduino core 3.x toolchain and verify all dependency APIs.
3. Check flash/PSRAM detection, boot mode, USB serial and available heap.
4. Integrate ST7789 initialization, RGB565 byte order, rotation, SPI clock and backlight.
5. Integrate the selected touch adapter; test corners, edges, reset and interrupt behavior.
6. Connect the shared `firmware/lvgl/PlayerUi.cpp` and its Swedish fonts/Lucide assets
   to the ESP32 LVGL 8.3.10 display/input port; run all LVGL work on one GUI task.
7. Integrate audio decoder and I²S pins; test a local MP3 at low volume before streaming.
8. Verify MicroSD mounting separately from display SPI; add a bounded download/cache implementation.
9. Provision Wi-Fi locally; verify CA trust, redirects, reconnection and real feed limits.
10. Verify NVS save/reboot recovery, power interruption, resume/seek capabilities, audio underruns,
    UI latency and multi-hour playback. Verify MP3, ADTS AAC and M4A separately.

No flashing, display, touch, PSRAM, SD, Wi-Fi, speaker or DAC operation has been tested.
