#pragma once
#include "lvgl.h"
#include "pod/Playback.hpp"
#include "pod/Ui.hpp"
#include <functional>

namespace pod::gui {
// Shared LVGL 8.3.10 UI. Call all methods from one GUI context.
// Demo service is explicit; real hardware supplies a different playback backend.
class PlayerUi {
public:
    using SaveCallback=std::function<void()>;
    void begin();
    void tick(uint32_t elapsedMs);
    void setScenario(int value);
    void reset();
    void restoreEpisode(int index,uint32_t position);
    void restoreSettings(int episode,int volume,bool resume,bool wifi);
    std::string snapshot() const;
    SaveCallback onSave;
private:
    enum Action { Back=1, Settings, PodcastRow, EpisodeRow, Current, Toggle, Back10, Forward10, Seek, Volume, Resume, Wifi, Retry, OtherEpisode };
    std::vector<Podcast> podcasts_;
    std::vector<Episode> episodes_;
    MockAudioPlayer audio_;
    Screen screen_=Screen::Podcasts, previous_=Screen::Podcasts;
    int podcast_=0, episode_=0, scenario_=0, volume_=65;
    bool resume_=true,wifi_=true,dirty_=true;
    uint32_t partialMs_=0,saveMs_=0;
    lv_obj_t *root_=nullptr,*elapsed_=nullptr,*remaining_=nullptr,*progress_=nullptr,*status_=nullptr,*volumeLabel_=nullptr,*playIcon_=nullptr;
    struct Binding { PlayerUi* owner; int action; int index; };
    std::vector<Binding> bindings_;
    static void event(lv_event_t* e);
    void dispatch(int action,int index,lv_obj_t* target);
    void draw();
    void update();
    void persist();
    bool online() const { return wifi_ && scenario_!=3; }
    bool playable() const { return online() || episodes_[episode_].downloaded; }
    void select(int index);
    lv_obj_t* button(lv_obj_t* parent,int x,int y,int w,int h,int action,int index=0);
    void bind(lv_obj_t*,int action,int index=0,lv_event_code_t code=LV_EVENT_CLICKED);
};
}
