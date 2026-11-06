#include "PlayerUi.hpp"
#include <emscripten.h>
#include <algorithm>
#include <deque>

namespace {
pod::gui::PlayerUi ui;
int pointerX=0,pointerY=0;bool pressed=false;
struct PointerSample {int x,y;bool down;};
std::deque<PointerSample> samples;
EM_JS(void,present,(int x,int y,int w,int h,const uint16_t* pixels),{
 const surface=Module.surface;
 const image=surface.image;
 let src=pixels>>1;
 for(let row=0;row<h;row++)for(let col=0;col<w;col++){
  const rgb=HEAPU16[src++],at=((y+row)*240+x+col)*4;
  image.data[at]=Math.round(((rgb>>11)&31)*255/31);
  image.data[at+1]=Math.round(((rgb>>5)&63)*255/63);
  image.data[at+2]=Math.round((rgb&31)*255/31);
  image.data[at+3]=255;
 }
 surface.context.putImageData(image,0,0,x,y,w,h);
 Module.flushCount=(Module.flushCount||0)+1;
});
EM_JS(void,save,(),{ if(Module.saveState)Module.saveState(); });
void flush(lv_disp_drv_t* display,const lv_area_t* area,lv_color_t* pixels){
 present(area->x1,area->y1,area->x2-area->x1+1,area->y2-area->y1+1,reinterpret_cast<const uint16_t*>(pixels));lv_disp_flush_ready(display);
}
void readPointer(lv_indev_drv_t*,lv_indev_data_t* data){if(!samples.empty()){const auto sample=samples.front();samples.pop_front();pointerX=sample.x;pointerY=sample.y;pressed=sample.down;}data->point.x=pointerX;data->point.y=pointerY;data->state=pressed?LV_INDEV_STATE_PR:LV_INDEV_STATE_REL;data->continue_reading=!samples.empty();}
void frame(){static double before=emscripten_get_now();const double now=emscripten_get_now();const auto ms=uint32_t(std::max(0.0,std::min(now-before,2000.0)));before=now;lv_tick_inc(ms);ui.tick(ms);lv_timer_handler();}
}
extern "C" {
EMSCRIPTEN_KEEPALIVE void demo_pointer(int x,int y,int down){const PointerSample sample{std::clamp(x,0,239),std::clamp(y,0,319),down!=0};if(samples.size()<64)samples.push_back(sample);else samples.back()=sample;}
EMSCRIPTEN_KEEPALIVE void demo_scenario(int value){ui.setScenario(value);}
EMSCRIPTEN_KEEPALIVE void demo_reset(){ui.reset();}
EMSCRIPTEN_KEEPALIVE const char* demo_snapshot(){static std::string json;json=ui.snapshot();return json.c_str();}
EMSCRIPTEN_KEEPALIVE void demo_restore_episode(int index,int seconds){if(seconds>=0)ui.restoreEpisode(index,uint32_t(seconds));}
EMSCRIPTEN_KEEPALIVE void demo_restore_settings(int episode,int volume,int resume,int wifi){ui.restoreSettings(episode,volume,resume!=0,wifi!=0);}
EMSCRIPTEN_KEEPALIVE void demo_save(){save();}
}
int main(){
 lv_init();static lv_color_t pixels[240*20];static lv_disp_draw_buf_t buffer;lv_disp_draw_buf_init(&buffer,pixels,nullptr,240*20);
 static lv_disp_drv_t display;lv_disp_drv_init(&display);display.hor_res=240;display.ver_res=320;display.draw_buf=&buffer;display.flush_cb=flush;lv_disp_drv_register(&display);
 static lv_indev_drv_t input;lv_indev_drv_init(&input);input.type=LV_INDEV_TYPE_POINTER;input.read_cb=readPointer;lv_indev_drv_register(&input);
 ui.begin();ui.onSave=[] {save();};emscripten_set_main_loop(frame,0,0);return 0;
}
