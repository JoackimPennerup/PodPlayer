#pragma once
#include "Interfaces.hpp"

namespace pod {
enum class Screen { Podcasts, Episodes, Player, Settings };
enum class UiState { Normal, Loading, Empty, Offline, PlaybackError, UnsupportedFormat };
enum class UiAction { None, Back, Settings, OpenPodcast, OpenEpisode, Toggle, Back10, Forward10, SeekAbsolute, Volume, ResumePreference, Retry };
enum class WidgetKind { Label, Button, Artwork, Slider, ScrollList, Toggle };
struct Widget {
    WidgetKind kind;
    int x,y,width,height;
    std::string text;
    UiAction action = UiAction::None;
    std::string id;
    int value = 0, max = 100;
};
struct UiSnapshot {
    Screen screen = Screen::Podcasts;
    UiState state = UiState::Normal;
    std::vector<Podcast> podcasts;
    std::vector<Episode> episodes;
    Podcast podcast;
    Episode episode;
    PlaybackState playback = PlaybackState::Paused;
    uint8_t volume = 65;
    bool resumeEnabled = true;
};
// Backend boundary for LVGL 8.3.10. All methods must run on the GUI task.
// No LVGL API calls are guessed while the headers are unavailable locally.
class LvglPort {
public:
    virtual ~LvglPort() = default;
    virtual Result beginFrame(uint16_t backgroundRgb565) = 0;
    virtual Result widget(const Widget&) = 0;
    virtual Result endFrame() = 0;
};
std::vector<Widget> layout(const UiSnapshot&);
Result renderLvgl(LvglPort&, const UiSnapshot&);
} // namespace pod
