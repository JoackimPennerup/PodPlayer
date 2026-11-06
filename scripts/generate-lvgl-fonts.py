#!/usr/bin/env python3
"""Generate LVGL 8 font callbacks from local DejaVu (Pillow required only here)."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'firmware/lvgl/assets'
OUT.mkdir(parents=True,exist_ok=True)
codes=list(range(32,127))+list(range(160,256))+[0x2013,0x2022,0x2039,0x2212]
text=['// Generated from DejaVu Sans. See DEJAVU-LICENSE.\n#include "Assets.hpp"\nnamespace pod::gui {\nstruct Glyph { uint32_t code,offset; uint16_t advance,w,h; int16_t x,y; };\nstruct Face { const Glyph* glyphs; size_t count; const uint8_t* pixels; };\nstatic const Glyph* find(const lv_font_t* f,uint32_t code) { const auto* d=static_cast<const Face*>(f->dsc); for(size_t i=0;i<d->count;++i) if(d->glyphs[i].code==code) return d->glyphs+i; return nullptr; }\nstatic bool describe(const lv_font_t* f,lv_font_glyph_dsc_t* d,uint32_t c,uint32_t) { const auto* g=find(f,c); if(!g) return false; d->adv_w=g->advance; d->box_w=g->w; d->box_h=g->h; d->ofs_x=g->x; d->ofs_y=g->y; d->bpp=8; d->is_placeholder=0; return true; }\nstatic const uint8_t* bitmap(const lv_font_t* f,uint32_t c) { const auto* g=find(f,c); return g?static_cast<const Face*>(f->dsc)->pixels+g->offset:nullptr; }\n']
for name,size,bold in [('small',10,False),('body',12,False),('title',14,True),('heading',18,True)]:
 font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans'+('-Bold' if bold else '')+'.ttf',size,layout_engine=ImageFont.Layout.BASIC)
 pixels=[];glyphs=[]
 for code in codes:
  char=chr(code);x0,y0,x1,y1=font.getbbox(char,anchor='ls');w=x1-x0;h=y1-y0
  im=Image.new('L',(max(w,1),max(h,1)))
  ImageDraw.Draw(im).text((-x0,-y0),char,font=font,fill=255,anchor='ls')
  glyphs.append((code,len(pixels),round(font.getlength(char)),w,h,x0,-y1))
  pixels+=list(im.getdata()) if w*h else []
 text+=['static const uint8_t '+name+'_pixels[]={'+','.join(map(str,pixels))+'};\n', 'static const Glyph '+name+'_glyphs[]={'+','.join('{'+','.join(map(str,g))+'}' for g in glyphs)+'};\n',f'static const Face {name}_face={{{name}_glyphs,{len(glyphs)},{name}_pixels}};\n',f'const lv_font_t font_{name}=[]{{ lv_font_t f={{}}; f.get_glyph_dsc=describe; f.get_glyph_bitmap=bitmap; f.line_height={size+4}; f.base_line=3; f.dsc=&{name}_face; return f; }}();\n']
text+=['}\n'];(OUT/'Fonts.cpp').write_text(''.join(text))
(OUT/'DEJAVU-LICENSE').write_text(Path('/usr/share/doc/fonts-dejavu-core/copyright').read_text())
