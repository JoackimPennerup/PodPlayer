#include "pod/Board.hpp"
#include "pod/Playback.hpp"

// Deliberate scaffold: no unverified vendor APIs, fake playback, GPIO writes or
// pin reservations by an assumed display library. setup/loop link with Arduino
// once an approved local toolchain is supplied, but no hardware is started yet.
// Native logic and UI plans are validated separately by scripts/test.sh.
#ifdef ARDUINO
pod::PendingAudioPlayer audio;
pod::board::St7789DisplayDriver display;
#if PODPLAYER_TOUCH_VERSION == 1
pod::board::Cst328TouchDriver touch;
#else
pod::board::Cst3530TouchDriver touch;
#endif
volatile pod::Result startupStatus = pod::Result::Unavailable;
void setup() {
    startupStatus = display.begin();
    // Failure remains visible in debugger. Do not report hardware ready.
}
void loop() {}
#endif
