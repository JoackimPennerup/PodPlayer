#include "pod/Ui.hpp"
#include <algorithm>

namespace pod {
namespace {
std::string time(uint32_t seconds) {
    const auto remainder=seconds%60;
    return std::to_string(seconds/60)+":"+(remainder<10 ? "0" : "")+std::to_string(remainder);
}
}
std::vector<Widget> layout(const UiSnapshot& s) {
    std::vector<Widget> out;
    auto add=[&](WidgetKind kind,int x,int y,int w,int h,std::string text,UiAction action=UiAction::None,std::string id="",int value=0,int max=100) {
        out.push_back({kind,x,y,w,h,std::move(text),action,std::move(id),value,max});
    };
    add(WidgetKind::Label,12,0,216,20,s.state==UiState::Offline ? "PodPlayer · Offline" : "PodPlayer");
    const auto title=s.screen==Screen::Podcasts ? "Mina poddar" : s.screen==Screen::Episodes ? s.podcast.title : s.screen==Screen::Player ? "Spelas nu" : "Inställningar";
    if(s.screen!=Screen::Podcasts) add(WidgetKind::Button,4,20,44,44,"‹",UiAction::Back);
    add(WidgetKind::Label,s.screen==Screen::Podcasts?12:48,20,s.screen==Screen::Podcasts?176:144,44,title);
    if(s.screen!=Screen::Settings) add(WidgetKind::Button,192,20,44,44,"⚙",UiAction::Settings);
    const bool message=s.state==UiState::Loading || s.state==UiState::Empty || s.state==UiState::PlaybackError || s.state==UiState::UnsupportedFormat || (s.screen==Screen::Player && s.state==UiState::Offline && !s.episode.downloaded);
    if(message) {
        const auto heading=s.state==UiState::Loading ? "Hämtar avsnitt" : s.state==UiState::Empty ? "Inga avsnitt ännu" : s.state==UiState::PlaybackError ? "Ljudet kunde inte spelas" : s.state==UiState::UnsupportedFormat ? "Ljudformatet stöds inte" : "Ingen nätanslutning";
        add(WidgetKind::Label,18,105,204,60,heading);
        add(WidgetKind::Button,48,190,144,44,"Försök igen",UiAction::Retry);
        return out;
    }
    if(s.screen==Screen::Podcasts || s.screen==Screen::Episodes) {
        add(WidgetKind::ScrollList,0,64,240,212,"");
        // Rows use content coordinates inside the ScrollList. Backend owns scrolling/clipping.
        if(s.screen==Screen::Podcasts) for(size_t i=0;i<s.podcasts.size();++i)
            add(WidgetKind::Button,0,int(i)*65,240,65,s.podcasts[i].title,UiAction::OpenPodcast,s.podcasts[i].id);
        if(s.screen==Screen::Episodes) for(size_t i=0;i<s.episodes.size();++i)
            add(WidgetKind::Button,0,int(i)*64,240,64,s.episodes[i].title+"\n"+s.episodes[i].publicationDate,UiAction::OpenEpisode,s.episodes[i].id);
        add(WidgetKind::Button,0,276,240,44,"Fortsätt lyssna",UiAction::Toggle);
    } else if(s.screen==Screen::Player) {
        add(WidgetKind::Artwork,12,66,68,68,s.podcast.title);
        add(WidgetKind::Label,90,66,138,18,s.podcast.title);
        add(WidgetKind::Label,90,84,138,58,s.episode.title);
        add(WidgetKind::Label,12,144,216,20,s.playback==PlaybackState::Playing ? "Spelar" : "Pausad");
        const auto position=std::min(s.episode.positionSeconds,s.episode.durationSeconds);
        add(WidgetKind::Slider,12,164,216,44,"Uppspelningsposition",UiAction::SeekAbsolute,"progress",int(position),int(s.episode.durationSeconds));
        add(WidgetKind::Label,12,208,104,18,time(position));
        add(WidgetKind::Label,128,208,100,18,"−"+time(s.episode.durationSeconds-position));
        add(WidgetKind::Button,25,238,44,44,"−10",UiAction::Back10);
        add(WidgetKind::Button,92,232,56,56,s.playback==PlaybackState::Playing ? "Pausa" : "Spela",UiAction::Toggle);
        add(WidgetKind::Button,171,238,44,44,"+10",UiAction::Forward10);
        add(WidgetKind::Label,12,292,216,24,"Volym "+std::to_string(s.volume)+" %");
    } else {
        add(WidgetKind::Label,12,64,216,24,"Volym");
        add(WidgetKind::Slider,12,88,216,44,"Volym",UiAction::Volume,"volume",s.volume);
        add(WidgetKind::Label,12,130,216,32,s.state==UiState::Offline ? "Wi-Fi · Ej ansluten" : "Wi-Fi · Ansluten");
        add(WidgetKind::Toggle,12,169,216,44,"Kom ihåg position",UiAction::ResumePreference,"resume",s.resumeEnabled);
        add(WidgetKind::Label,12,230,216,64,"PodPlayer 0.1\nESP32-S3 · 240 × 320");
    }
    return out;
}
Result renderLvgl(LvglPort& port,const UiSnapshot& state) {
    auto result=port.beginFrame(0xf7be);
    if(result!=Result::Ok) return result;
    for(const auto& w:layout(state)) { result=port.widget(w); if(result!=Result::Ok) return result; }
    return port.endFrame();
}
} // namespace pod
