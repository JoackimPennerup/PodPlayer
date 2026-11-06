#!/usr/bin/env python3
"""Compile real LVGL 8.3.10 + shared C++ UI. No dependencies downloaded here."""
import concurrent.futures, hashlib, os, pathlib, shutil, subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
LVGL=pathlib.Path(os.environ.get('LVGL_DIR','/workspace/deps/lvgl-8.3.10')).resolve()
EMSDK=pathlib.Path(os.environ.get('EMSDK','/workspace/deps/emsdk'))
EMCC=os.environ.get('EMCC',str(EMSDK/'upstream/emscripten/emcc'))
EMXX=os.environ.get('EMXX',str(EMSDK/'upstream/emscripten/em++'))
if not (LVGL/'lvgl.h').is_file():raise SystemExit('LVGL_DIR must point to LVGL 8.3.10 sources. See docs/lvgl-web.md.')
if not pathlib.Path(EMCC).is_file() and not shutil.which(EMCC):raise SystemExit('Emscripten missing. See docs/lvgl-web.md.')
header=(LVGL/'lvgl.h').read_text()
for macro,value in [('LVGL_VERSION_MAJOR',8),('LVGL_VERSION_MINOR',3),('LVGL_VERSION_PATCH',10)]:
 if not any(line.split()==['#define',macro,str(value)] for line in header.splitlines()):raise SystemExit('Requires exact LVGL 8.3.10')
subprocess.run(['python3',str(ROOT/'scripts/generate-lvgl-fixtures.py')],check=True)
BUILD=ROOT/'build/lvgl-web';BUILD.mkdir(parents=True,exist_ok=True)
OUT=ROOT/'web/dist';OUT.mkdir(parents=True,exist_ok=True)
flags=['-O2','-DLV_CONF_INCLUDE_SIMPLE','-I'+str(LVGL),'-I'+str(ROOT/'firmware/lvgl'),'-I'+str(ROOT/'firmware/include')]
sources=sorted((LVGL/'src').rglob('*.c'))+[ROOT/p for p in ['firmware/lvgl/PlayerUi.cpp','firmware/lvgl/assets/Icons.cpp','firmware/lvgl/assets/Fonts.cpp','web/main.cpp']]
headers=list((ROOT/'firmware/lvgl').rglob('*.hpp'))+[ROOT/'firmware/lvgl/lv_conf.h']
config=hashlib.sha256((' '.join(flags)+str(LVGL)+subprocess.check_output([EMCC,'--version'],text=True)+''.join(p.read_text() for p in headers)).encode()).hexdigest()
marker=BUILD/'config.sha256';rebuild=not marker.exists() or marker.read_text()!=config

def compile(source):
 obj=BUILD/(hashlib.sha256(str(source).encode()).hexdigest()[:16]+'.o')
 if rebuild or not obj.exists() or obj.stat().st_mtime<source.stat().st_mtime:
  compiler=EMXX if source.suffix=='.cpp' else EMCC
  command=[compiler,*flags,*(['-std=c++17','-Wall','-Wextra'] if source.suffix=='.cpp' else []),'-c',str(source),'-o',str(obj)]
  subprocess.run(command,check=True)
 return str(obj)
print(f'Building {len(sources)} sources with {min(4,os.cpu_count() or 1)} workers',flush=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=min(4,os.cpu_count() or 1)) as pool:objects=list(pool.map(compile,sources))
marker.write_text(config)
subprocess.run([EMXX,*objects,'-O2','-sALLOW_MEMORY_GROWTH=1','-sINITIAL_MEMORY=16777216','-sENVIRONMENT=web','-sMODULARIZE=1','-sEXPORT_NAME=createPodPlayer','-sEXPORTED_RUNTIME_METHODS=["UTF8ToString"]','-sFILESYSTEM=0','-o',str(OUT/'podplayer.js')],check=True)
shutil.copyfile(LVGL/'LICENCE.txt',OUT/'LVGL-LICENSE.txt')
for license_name in ['DEJAVU-LICENSE','LUCIDE-LICENSE']:
 shutil.copyfile(ROOT/'firmware/lvgl/assets'/license_name,OUT/(license_name+'.txt'))
print('Built web/dist/podplayer.js + podplayer.wasm',flush=True)
