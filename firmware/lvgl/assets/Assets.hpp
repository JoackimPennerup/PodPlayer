#pragma once
#include "lvgl.h"
namespace pod::gui {
extern const lv_font_t font_small, font_body, font_title, font_heading;
const lv_img_dsc_t* icon(const char* name,int size=20);
}
