#include "PlayerUi.hpp"
#include "DemoFixtures.hpp"
#include "assets/Assets.hpp"
#include <algorithm>
#include <cstdio>

namespace pod::gui {
namespace {
constexpr uint32_t Background=0xf6f7f2,Ink=0x213132,Muted=0x627260,Accent=0x12685b,Line=0xdce2dc;
constexpr uint32_t covers[]={0x914b39,0x355e72,0x667344};
const char* initials[]={"Mr","Nv","Sk"};
std::string jsonString(const std::string& value) { std::string out="\"";for(unsigned char c:value){if(c=='"'||c=='\\'){out+='\\';out+=char(c);}else if(c>=32)out+=char(c);}return out+"\""; }
std::string time(uint32_t sec) { char b[24]; std::snprintf(b,sizeof b,"%u:%02u",sec/60,sec%60); return b; }
void box(lv_obj_t* o,int x,int y,int w,int h) { lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h); }
void plain(lv_obj_t* o) { lv_obj_remove_style_all(o);lv_obj_set_style_text_color(o,lv_color_hex(Ink),0);lv_obj_set_style_text_font(o,&font_body,0); }
lv_obj_t* label(lv_obj_t* p,int x,int y,int w,const std::string& text,const lv_font_t* font=&font_body,uint32_t color=Ink) {
 auto* o=lv_label_create(p);plain(o);lv_obj_set_pos(o,x,y);lv_obj_set_width(o,w);lv_label_set_text(o,text.c_str());lv_obj_set_style_text_font(o,font,0);lv_obj_set_style_text_color(o,lv_color_hex(color),0);lv_obj_clear_flag(o,LV_OBJ_FLAG_CLICKABLE);return o;
}
lv_obj_t* image(lv_obj_t* p,const char* name,int size,int x,int y,uint32_t color=Ink) {
 auto* o=lv_img_create(p);plain(o);lv_img_set_src(o,icon(name,size));lv_obj_set_pos(o,x,y);lv_obj_set_style_img_recolor(o,lv_color_hex(color),0);lv_obj_set_style_img_recolor_opa(o,LV_OPA_COVER,0);lv_obj_clear_flag(o,LV_OBJ_FLAG_CLICKABLE);return o;
}
void background(lv_obj_t* o,uint32_t color,int radius=0) {lv_obj_set_style_bg_color(o,lv_color_hex(color),0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);lv_obj_set_style_radius(o,radius,0);}
void rule(lv_obj_t* parent,int y) {auto* o=lv_obj_create(parent);plain(o);box(o,12,y,216,1);background(o,Line);}
void cover(lv_obj_t* parent,int x,int y,int size,int pod) {
 auto* o=lv_obj_create(parent);plain(o);box(o,x,y,size,size);background(o,covers[pod%3],8);lv_obj_clear_flag(o,LV_OBJ_FLAG_SCROLLABLE);
 label(o,6,5,size-12,initials[pod%3],&font_heading,0xfff9e9);label(o,6,size-16,size-12,"PODD",&font_small,0xfff9e9);
}
}
void PlayerUi::begin() {
 bindings_.reserve(80);reset();draw();
}
void PlayerUi::select(int index) {
 if(index<0 || size_t(index)>=episodes_.size())return;
 if(!episodes_.empty())episodes_[episode_].positionSeconds=audio_.getPlaybackPosition();
 episode_=index;auto& e=episodes_[episode_];
 audio_=MockAudioPlayer(e.durationSeconds);
 audio_.play(e.audioUrl);audio_.setVolume(uint8_t(volume_));
 if(resume_)audio_.seekRelativeSeconds(int32_t(e.positionSeconds));
 if(audio_.getPlaybackState()==PlaybackState::Playing)audio_.pause();
 for(size_t i=0;i<podcasts_.size();++i)if(podcasts_[i].id==e.podcastId)podcast_=int(i);
 partialMs_=0;
}
void PlayerUi::persist() {episodes_[episode_].positionSeconds=audio_.getPlaybackPosition();if(onSave)onSave();saveMs_=0;}
void PlayerUi::restoreEpisode(int index,uint32_t position) {if(index>=0&&size_t(index)<episodes_.size())episodes_[index].positionSeconds=std::min(position,episodes_[index].durationSeconds);}
void PlayerUi::restoreSettings(int episode,int volume,bool resume,bool wifi) {
 volume_=std::clamp(volume,0,100);resume_=resume;wifi_=wifi;
 // Avoid overwriting restored position when select() saves the previous selection.
 const auto restored=episodes_;select(episode);episodes_=restored;
 audio_=MockAudioPlayer(episodes_[episode_].durationSeconds);audio_.play(episodes_[episode_].audioUrl);
 if(resume_)audio_.seekRelativeSeconds(int32_t(episodes_[episode_].positionSeconds));
 if(audio_.getPlaybackState()==PlaybackState::Playing)audio_.pause();audio_.setVolume(uint8_t(volume_));dirty_=true;
}
void PlayerUi::bind(lv_obj_t* object,int action,int index,lv_event_code_t code) {
 bindings_.push_back({this,action,index});lv_obj_add_event_cb(object,event,code,&bindings_.back());
}
lv_obj_t* PlayerUi::button(lv_obj_t* p,int x,int y,int w,int h,int action,int index) {
 auto* o=lv_btn_create(p);plain(o);box(o,x,y,w,h);lv_obj_set_style_radius(o,8,0);lv_obj_set_style_bg_color(o,lv_color_hex(0xe4eae2),LV_STATE_PRESSED);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,LV_STATE_PRESSED);lv_obj_clear_flag(o,LV_OBJ_FLAG_SCROLLABLE);bind(o,action,index);return o;
}
void PlayerUi::event(lv_event_t* event) {auto* b=static_cast<Binding*>(lv_event_get_user_data(event));b->owner->dispatch(b->action,b->index,lv_event_get_target(event));}
void PlayerUi::dispatch(int action,int index,lv_obj_t* target) {
 switch(action) {
 case Back: screen_=screen_==Screen::Settings?previous_:screen_==Screen::Player?Screen::Episodes:Screen::Podcasts;dirty_=true;break;
 case Settings: previous_=screen_;screen_=Screen::Settings;dirty_=true;break;
 case PodcastRow:podcast_=index;screen_=Screen::Episodes;dirty_=true;break;
 case EpisodeRow:select(index);screen_=Screen::Player;dirty_=true;break;
 case Current:for(size_t i=0;i<podcasts_.size();++i)if(podcasts_[i].id==episodes_[episode_].podcastId)podcast_=int(i);screen_=Screen::Player;dirty_=true;break;
 case Toggle:
  if(!playable()){screen_=Screen::Player;dirty_=true;break;}
  if(audio_.getPlaybackState()==PlaybackState::Playing)audio_.pause();
  else if(audio_.getPlaybackState()==PlaybackState::Completed)audio_.play(episodes_[episode_].audioUrl);
  else audio_.resume();
  partialMs_=0;dirty_=true;break;
 case Back10:audio_.seekRelativeSeconds(-10);update();break;
 case Forward10:audio_.seekRelativeSeconds(10);update();break;
 case Seek:audio_.seekRelativeSeconds(lv_slider_get_value(target)-int32_t(audio_.getPlaybackPosition()));update();break;
 case Volume:volume_=lv_slider_get_value(target);audio_.setVolume(uint8_t(volume_));if(volumeLabel_)lv_label_set_text_fmt(volumeLabel_,"%d %%",volume_);break;
 case Resume:resume_=lv_obj_has_state(target,LV_STATE_CHECKED);break;
 case Wifi:wifi_=!wifi_;scenario_=0;if(!playable()&&audio_.getPlaybackState()==PlaybackState::Playing)audio_.pause();dirty_=true;break;
 case Retry:scenario_=0;if(!wifi_)wifi_=true;dirty_=true;break;
 case OtherEpisode:scenario_=0;screen_=Screen::Episodes;dirty_=true;break;
 }
 persist();
}
void PlayerUi::setScenario(int scenario) {
 scenario_=std::clamp(scenario,0,5);if(audio_.getPlaybackState()==PlaybackState::Playing)audio_.pause();
 screen_=scenario_>=4?Screen::Player:scenario_?Screen::Episodes:Screen::Podcasts;dirty_=true;persist();
}
void PlayerUi::reset() {podcasts_=demoPodcasts();episodes_=demoEpisodes();episode_=0;podcast_=0;volume_=65;resume_=wifi_=true;scenario_=0;screen_=Screen::Podcasts;audio_=MockAudioPlayer(episodes_[0].durationSeconds);audio_.play(episodes_[0].audioUrl);audio_.seekRelativeSeconds(int32_t(episodes_[0].positionSeconds));audio_.pause();dirty_=true;persist();}
void PlayerUi::tick(uint32_t elapsedMs) {
 saveMs_+=elapsedMs;
 if(audio_.getPlaybackState()==PlaybackState::Playing) {
  partialMs_+=std::min(elapsedMs,2000U);
  if(partialMs_>=1000){audio_.tick(partialMs_/1000);partialMs_%=1000;update();if(audio_.getPlaybackState()==PlaybackState::Completed){dirty_=true;persist();}}
  if(saveMs_>=30000)persist();
 }
 if(dirty_)draw();
}
void PlayerUi::update() {
 const auto pos=audio_.getPlaybackPosition();episodes_[episode_].positionSeconds=pos;
 if(elapsed_)lv_label_set_text(elapsed_,time(pos).c_str());
 if(remaining_)lv_label_set_text(remaining_,("−"+time(episodes_[episode_].durationSeconds-pos)).c_str());
 if(progress_&&!lv_obj_has_state(progress_,LV_STATE_PRESSED))lv_slider_set_value(progress_,int32_t(pos),LV_ANIM_OFF);
 if(playIcon_)lv_img_set_src(playIcon_,icon(audio_.getPlaybackState()==PlaybackState::Playing?"pause":"play",28));
 if(status_)lv_label_set_text(status_,audio_.getPlaybackState()==PlaybackState::Playing?"Spelar · Demo":audio_.getPlaybackState()==PlaybackState::Completed?"Lyssnat · Demo":"Pausad · Demo");
}
void PlayerUi::draw() {
 dirty_=false;
 // Deferred from event callbacks: old object user data stays alive until handlers return.
 if(root_)lv_obj_del(root_);bindings_.clear();elapsed_=remaining_=progress_=status_=volumeLabel_=playIcon_=nullptr;
 root_=lv_obj_create(lv_scr_act());plain(root_);box(root_,0,0,240,320);background(root_,Background);lv_obj_clear_flag(root_,LV_OBJ_FLAG_SCROLLABLE);
 label(root_,12,4,145,"PodPlayer",&font_small,Muted);image(root_,online()?"wifi":"offline",12,200,4,Muted);image(root_,"battery",12,218,4,Muted);
 if(screen_!=Screen::Podcasts){auto* back=button(root_,2,20,44,44,Back);image(back,"back",20,12,12);}
 const std::string heading=screen_==Screen::Podcasts?"Mina poddar":screen_==Screen::Episodes?podcasts_[podcast_].title:screen_==Screen::Player?"Spelas nu":"Inställningar";
 auto* title=label(root_,screen_==Screen::Podcasts?12:48,33,screen_==Screen::Podcasts?176:140,heading,screen_==Screen::Podcasts?&font_heading:&font_title);lv_label_set_long_mode(title,LV_LABEL_LONG_DOT);
 if(screen_!=Screen::Settings){auto* settings=button(root_,192,20,44,44,Settings);image(settings,"settings",20,12,12);}
 const bool error=screen_==Screen::Player&&(scenario_>=4||!playable());
 const bool loading=screen_==Screen::Episodes&&scenario_==1,empty=screen_==Screen::Episodes&&scenario_==2;
 if(error||loading||empty){
  if(loading){auto* spinner=lv_spinner_create(root_,1000,60);box(spinner,106,98,28,28);lv_obj_set_style_arc_color(spinner,lv_color_hex(Line),LV_PART_MAIN);lv_obj_set_style_arc_width(spinner,3,LV_PART_MAIN);lv_obj_set_style_arc_color(spinner,lv_color_hex(Accent),LV_PART_INDICATOR);lv_obj_set_style_arc_width(spinner,3,LV_PART_INDICATOR);}else image(root_,empty?"list":scenario_==3?"offline":"error",28,106,98,Accent);
  const char* message=loading?"Hämtar avsnitt":empty?"Inga avsnitt ännu":scenario_==4?"Ljudet kunde inte spelas":scenario_==5?"Ljudformatet stöds inte":"Ingen nätanslutning";
  auto* h=label(root_,18,141,204,message,&font_title);lv_obj_set_style_text_align(h,LV_TEXT_ALIGN_CENTER,0);
  const char* description=loading?"Det tar bara en liten stund.":empty?"Nya avsnitt visas här när podden publicerar dem.":scenario_==5?"Välj ett annat avsnitt.":!playable()?"Avsnittet är inte sparat. Anslut till Wi-Fi för att lyssna.":"Försök igen. Din position är sparad.";
  auto* d=label(root_,20,181,200,description,&font_body,Muted);lv_obj_set_style_text_align(d,LV_TEXT_ALIGN_CENTER,0);
  auto* retry=button(root_,36,245,168,44,scenario_==5?OtherEpisode:Retry);background(retry,Accent,8);label(retry,8,14,152,loading?"Visa demoavsnitt":scenario_==5?"Visa avsnitt":"Försök igen",&font_body,0xffffff);return;
 }
 if(screen_==Screen::Podcasts||screen_==Screen::Episodes){
  int top=64;
  if(!online()){label(root_,12,66,216,"Offline · sparade avsnitt kan spelas",&font_small,0x704c1f);top=85;}
  auto* list=lv_obj_create(root_);plain(list);box(list,0,top,240,276-top);lv_obj_set_scroll_dir(list,LV_DIR_VER);lv_obj_set_scrollbar_mode(list,LV_SCROLLBAR_MODE_AUTO);lv_obj_set_style_bg_color(list,lv_color_hex(Line),LV_PART_SCROLLBAR);lv_obj_set_style_bg_opa(list,LV_OPA_COVER,LV_PART_SCROLLBAR);lv_obj_set_style_width(list,3,LV_PART_SCROLLBAR);
  if(screen_==Screen::Podcasts){
   label(list,12,0,216,"DITT BIBLIOTEK",&font_small,Muted);
   for(size_t i=0;i<podcasts_.size();++i){auto* row=button(list,0,20+int(i)*64,240,64,PodcastRow,int(i));cover(row,12,11,42,int(i));label(row,64,13,148,podcasts_[i].title,&font_body);label(row,64,36,140,std::to_string(std::count_if(episodes_.begin(),episodes_.end(),[&](const Episode& e){return e.podcastId==podcasts_[i].id;}))+" avsnitt",&font_small,Muted);image(row,"chevron",12,216,25,Muted);rule(row,63);}
  }else{
   int rowIndex=0;
   for(size_t i=0;i<episodes_.size();++i){const auto& e=episodes_[i];if(e.podcastId!=podcasts_[podcast_].id)continue;
    auto* row=button(list,0,rowIndex++*76,240,76,EpisodeRow,int(i));auto* t=label(row,12,8,216,e.title,&font_title);lv_obj_set_height(t,36);lv_label_set_long_mode(t,LV_LABEL_LONG_DOT);
    label(row,12,45,216,e.publicationDate+" · "+std::to_string(e.durationSeconds/60)+" min"+(e.downloaded?" · Sparat":""),&font_small,Muted);
    if(e.positionSeconds)label(row,12,61,216,e.positionSeconds>=e.durationSeconds?"Lyssnat":time(e.durationSeconds-e.positionSeconds)+" kvar",&font_small,Accent);rule(row,75);
   }
  }
  auto* mini=button(root_,0,276,196,44,Current);background(mini,0xe8eee4);image(mini,"volume",20,10,12,Accent);auto* t=label(mini,38,7,151,episodes_[episode_].title,&font_small);lv_label_set_long_mode(t,LV_LABEL_LONG_DOT);label(mini,38,24,151,"Fortsätt lyssna",&font_small,Muted);
  auto* toggle=button(root_,196,276,44,44,Toggle);background(toggle,0xe8eee4);image(toggle,audio_.getPlaybackState()==PlaybackState::Playing?"pause":"play",20,12,12,Accent);return;
 }
 if(screen_==Screen::Player){
  const auto& e=episodes_[episode_];cover(root_,12,68,68,podcast_);label(root_,90,68,138,podcasts_[podcast_].title,&font_small,Accent);auto* t=label(root_,90,89,138,e.title,&font_title);lv_obj_set_height(t,54);lv_label_set_long_mode(t,LV_LABEL_LONG_DOT);
  status_=label(root_,12,146,216,audio_.getPlaybackState()==PlaybackState::Playing?"Spelar · Demo":"Pausad · Demo",&font_small,Muted);
  progress_=lv_slider_create(root_);box(progress_,18,178,204,5);lv_obj_set_ext_click_area(progress_,18);lv_slider_set_range(progress_,0,int32_t(e.durationSeconds));lv_slider_set_value(progress_,int32_t(audio_.getPlaybackPosition()),LV_ANIM_OFF);background(progress_,Line,4);lv_obj_set_style_bg_opa(progress_,LV_OPA_COVER,LV_PART_INDICATOR);lv_obj_set_style_bg_opa(progress_,LV_OPA_COVER,LV_PART_KNOB);lv_obj_set_style_bg_color(progress_,lv_color_hex(Accent),LV_PART_INDICATOR);lv_obj_set_style_bg_color(progress_,lv_color_hex(Accent),LV_PART_KNOB);lv_obj_set_style_pad_all(progress_,5,LV_PART_KNOB);lv_obj_set_style_radius(progress_,LV_RADIUS_CIRCLE,LV_PART_KNOB);lv_obj_set_style_radius(progress_,4,LV_PART_INDICATOR);bind(progress_,Seek,0,LV_EVENT_VALUE_CHANGED);
  elapsed_=label(root_,12,202,104,time(audio_.getPlaybackPosition()),&font_small,Muted);remaining_=label(root_,128,202,100,"−"+time(e.durationSeconds-audio_.getPlaybackPosition()),&font_small,Muted);lv_obj_set_style_text_align(remaining_,LV_TEXT_ALIGN_RIGHT,0);
  auto* back=button(root_,25,232,44,44,Back10);image(back,"back10",28,8,8);label(back,15,16,22,"10",&font_small);
  auto* play=button(root_,92,226,56,56,Toggle);background(play,Accent,28);playIcon_=image(play,audio_.getPlaybackState()==PlaybackState::Playing?"pause":"play",28,14,14,0xffffff);
  auto* forward=button(root_,171,232,44,44,Forward10);image(forward,"forward10",28,8,8);label(forward,15,16,22,"10",&font_small);
  image(root_,"volume",12,51,298,Muted);label(root_,68,298,160,std::to_string(volume_)+" % · Simulerat ljud",&font_small,Muted);return;
 }
 label(root_,12,72,120,"Volym");volumeLabel_=label(root_,168,72,60,std::to_string(volume_)+" %",&font_body,Muted);
 auto* slider=lv_slider_create(root_);box(slider,18,110,204,5);lv_obj_set_ext_click_area(slider,18);lv_slider_set_range(slider,0,100);lv_slider_set_value(slider,volume_,LV_ANIM_OFF);background(slider,Line,4);lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_INDICATOR);lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_KNOB);lv_obj_set_style_bg_color(slider,lv_color_hex(Accent),LV_PART_INDICATOR);lv_obj_set_style_bg_color(slider,lv_color_hex(Accent),LV_PART_KNOB);lv_obj_set_style_pad_all(slider,5,LV_PART_KNOB);lv_obj_set_style_radius(slider,LV_RADIUS_CIRCLE,LV_PART_KNOB);lv_obj_set_style_radius(slider,4,LV_PART_INDICATOR);bind(slider,Volume,0,LV_EVENT_VALUE_CHANGED);rule(root_,140);
 auto* wifi=button(root_,0,145,240,44,Wifi);label(wifi,12,13,70,"Wi-Fi");label(wifi,123,13,105,online()?"Ansluten":"Ej ansluten",&font_body,Accent);rule(root_,192);
 label(root_,12,212,166,"Kom ihåg position");auto* sw=lv_switch_create(root_);box(sw,190,210,34,20);lv_obj_set_ext_click_area(sw,12);background(sw,0xa5b0a2,12);lv_obj_set_style_bg_opa(sw,LV_OPA_COVER,LV_PART_INDICATOR);lv_obj_set_style_radius(sw,12,LV_PART_INDICATOR);lv_obj_set_style_bg_opa(sw,LV_OPA_COVER,LV_PART_KNOB);lv_obj_set_style_bg_color(sw,lv_color_hex(0xffffff),LV_PART_KNOB);lv_obj_set_style_radius(sw,12,LV_PART_KNOB);if(resume_)lv_obj_add_state(sw,LV_STATE_CHECKED);lv_obj_set_style_bg_color(sw,lv_color_hex(Accent),LV_PART_INDICATOR|LV_STATE_CHECKED);bind(sw,Resume,0,LV_EVENT_VALUE_CHANGED);rule(root_,245);
 label(root_,12,257,216,"PodPlayer 0.2 · LVGL 8.3.10",&font_small,Muted);label(root_,12,275,216,"ESP32-S3 · 240 × 320",&font_small,Muted);label(root_,12,293,216,"Wi-Fi och ljud simuleras",&font_small,Muted);
}
std::string PlayerUi::snapshot() const {
 std::string result="{\"screen\":"+std::to_string(int(screen_))+",\"scenario\":"+std::to_string(scenario_)+",\"episode\":"+std::to_string(episode_)+",\"podcast\":"+std::to_string(podcast_)+",\"position\":"+std::to_string(audio_.getPlaybackPosition())+",\"playing\":"+(audio_.getPlaybackState()==PlaybackState::Playing?"true":"false")+",\"volume\":"+std::to_string(volume_)+",\"resume\":"+(resume_?"true":"false")+",\"wifi\":"+(wifi_?"true":"false")+",\"positions\":[";
 for(size_t i=0;i<episodes_.size();++i){if(i)result+=",";result+=std::to_string(i==size_t(episode_)?audio_.getPlaybackPosition():episodes_[i].positionSeconds);}result+="],\"ids\":[";for(size_t i=0;i<episodes_.size();++i){if(i)result+=",";result+=jsonString(episodes_[i].id);}return result+"],\"episodeId\":"+jsonString(episodes_[episode_].id)+"}";
}
}
