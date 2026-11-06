#pragma once
#include "Interfaces.hpp"

namespace pod::board {
// User-supplied SKU 27690 pin map; physical verification remains required.
constexpr int Width = 240, Height = 320;
constexpr int LcdMosi = 45, LcdClock = 40, LcdCs = 42, LcdDc = 41, LcdReset = 39, Backlight = 5;
constexpr int TouchSda = 1, TouchScl = 3, TouchInt = 4, TouchReset = 2;
constexpr int AudioBclk = 48, AudioLrck = 38, AudioDout = 47;
constexpr int SdMiso = 16, SdMosi = 17, SdClock = 14, SdCs = 21;
enum class TouchRevision { Unconfirmed, V1_Cst328, V2_Cst3530 };
constexpr uint8_t Cst328Address = 0x1a, Cst3530Address = 0x58;
#ifndef PODPLAYER_TOUCH_VERSION
#define PODPLAYER_TOUCH_VERSION 2
#endif
static_assert(PODPLAYER_TOUCH_VERSION == 1 || PODPLAYER_TOUCH_VERSION == 2, "Touch version must be 1 or 2");
constexpr TouchRevision DefaultTouchRevision = PODPLAYER_TOUCH_VERSION == 1
    ? TouchRevision::V1_Cst328 : TouchRevision::V2_Cst3530;
struct TouchSelection {
    TouchRevision revision;
    uint8_t address;
    bool detected;
    Result result;
};
// Probe is supplied by a verified I2C adapter; an ACK identifies an address, not a chip ID.
template<class Probe> TouchSelection selectTouch(Probe probe, bool autodetect = true) {
    if (autodetect) {
        const bool v1 = probe(Cst328Address), v2 = probe(Cst3530Address);
        if (v1 && v2) return {TouchRevision::Unconfirmed, 0, false, Result::InvalidData};
        if (v1) return {TouchRevision::V1_Cst328, Cst328Address, true, Result::Ok};
        if (v2) return {TouchRevision::V2_Cst3530, Cst3530Address, true, Result::Ok};
    }
    // No ACK or probing disabled: explicit compile-time fallback, not successful detection.
    return {DefaultTouchRevision, PODPLAYER_TOUCH_VERSION == 1 ? Cst328Address : Cst3530Address, false, Result::Ok};
}

// Deliberately fail closed: controller protocols cannot be verified offline.
// Replace each adapter after inspecting vendor headers for the actual revision.
template<TouchRevision Revision> class PendingTouchDriver final : public TouchDriver {
public:
    Result begin() override { return Result::Unavailable; }
    Result read(TouchPoint& point) override { point = {}; return Result::Unavailable; }
    static constexpr TouchRevision revision = Revision;
};
using Cst328TouchDriver = PendingTouchDriver<TouchRevision::V1_Cst328>;
using Cst3530TouchDriver = PendingTouchDriver<TouchRevision::V2_Cst3530>;
class St7789DisplayDriver final : public DisplayDriver {
public:
    Result begin() override { return Result::Unavailable; }
    Result flush(uint16_t, uint16_t, uint16_t, uint16_t, const uint16_t*) override {
        return Result::Unavailable;
    }
};
} // namespace pod::board
