#!/usr/bin/env python3
import json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
podcasts=json.loads((root/'fixtures/podcasts.json').read_text())
episodes=json.loads((root/'fixtures/episodes.json').read_text())
s=lambda x:json.dumps(x,ensure_ascii=False)
lines=['// Generated from fixtures/*.json.\n#pragma once\n#include "pod/Model.hpp"\nnamespace pod::gui {\ninline std::vector<Podcast> demoPodcasts() { return {']
lines.append(','.join('{'+','.join(s(p[k]) for k in ['id','title','feedUrl','artworkUrl'])+'}' for p in podcasts))
lines+=['}; }\ninline std::vector<Episode> demoEpisodes() { std::vector<Episode> out;\n']
for e in episodes:
 lines+=['{ Episode e;'+''.join('e.'+dest+'='+s(e[src])+';' for src,dest in [('id','id'),('podcastId','podcastId'),('title','title'),('pubDate','publicationDate'),('audioUrl','audioUrl'),('mimeType','mimeType')])+f'e.durationSeconds={e["duration"]};e.positionSeconds={e["position"]};e.downloaded={str(e["downloaded"]).lower()};out.push_back(e);}}\n']
lines+=['return out; }\n}\n'];(root/'firmware/lvgl/DemoFixtures.hpp').write_text(''.join(lines))
