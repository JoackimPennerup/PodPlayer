// Build-time only: use an already installed Sharp to rasterize the local Lucide subset.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const sharp=require(process.env.SHARP_MODULE || 'sharp');
const root=path.resolve(__dirname,'..'),assets=path.join(root,'firmware/lvgl/assets');
vm.runInThisContext(fs.readFileSync(path.join(root,'mock/vendor/lucide/icons.js'),'utf8'));
(async()=>{
 let text='// Generated from Lucide 1.8.0. See LUCIDE-LICENSE.\n#include "Assets.hpp"\n#include <cstring>\nnamespace pod::gui {\n';
 const declarations=[];
 for(const [name,icon] of Object.entries(POD_ICONS)) for(const size of [12,20,28]){
  const svg=`<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">${icon.svg}</svg>`;
  const {data}=await sharp(Buffer.from(svg)).resize(size,size).ensureAlpha().raw().toBuffer({resolveWithObject:true});
  const alpha=[];for(let i=3;i<data.length;i+=4)alpha.push(data[i]);
  const id=name+'_'+size;
  text+=`static const uint8_t ${id}_pixels[]={${alpha.join(',')}};\nstatic const lv_img_dsc_t ${id}=[] { lv_img_dsc_t d={}; d.header.cf=LV_IMG_CF_ALPHA_8BIT; d.header.w=${size}; d.header.h=${size}; d.data_size=${alpha.length}; d.data=${id}_pixels; return d; }();\n`;
  declarations.push(`if(size==${size} && std::strcmp(name,"${name}")==0) return &${id};`);
 }
 text+='const lv_img_dsc_t* icon(const char* name,int size) {\n'+declarations.join('\n')+'\nreturn nullptr; }\n}\n';
 fs.writeFileSync(path.join(assets,'Icons.cpp'),text);
 fs.copyFileSync(path.join(root,'mock/vendor/lucide/LICENSE'),path.join(assets,'LUCIDE-LICENSE'));
})();
