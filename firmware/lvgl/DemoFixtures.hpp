// Generated from fixtures/*.json.
#pragma once
#include "pod/Model.hpp"
namespace pod::gui {
inline std::vector<Podcast> demoPodcasts() { return {{"mellan-raderna","Mellan raderna","https://example.invalid/mellan-raderna.xml",""},{"nyfiken","Nyfiken på världen","https://example.invalid/nyfiken.xml",""},{"sondag","Söndag i köket","https://example.invalid/sondag.xml",""}}; }
inline std::vector<Episode> demoEpisodes() { std::vector<Episode> out;
{ Episode e;e.id="mellan-raderna-1";e.podcastId="mellan-raderna";e.title="När staden somnar";e.publicationDate="2026-11-05";e.audioUrl="https://example.invalid/audio/mellan-raderna0.mp3";e.mimeType="audio/mpeg";e.durationSeconds=2382;e.positionSeconds=442;e.downloaded=true;out.push_back(e);}
{ Episode e;e.id="mellan-raderna-2";e.podcastId="mellan-raderna";e.title="Ett brev från förr";e.publicationDate="2026-11-03";e.audioUrl="https://example.invalid/audio/mellan-raderna1.mp3";e.mimeType="audio/mpeg";e.durationSeconds=1940;e.positionSeconds=0;e.downloaded=false;out.push_back(e);}
{ Episode e;e.id="mellan-raderna-3";e.podcastId="mellan-raderna";e.title="Det vi lämnar efter oss";e.publicationDate="2026-11-01";e.audioUrl="https://example.invalid/audio/mellan-raderna2.mp3";e.mimeType="audio/mpeg";e.durationSeconds=2715;e.positionSeconds=0;e.downloaded=false;out.push_back(e);}
{ Episode e;e.id="mellan-raderna-4";e.podcastId="mellan-raderna";e.title="En plats att komma hem till";e.publicationDate="2026-10-30";e.audioUrl="https://example.invalid/audio/mellan-raderna3.mp3";e.mimeType="audio/mpeg";e.durationSeconds=1820;e.positionSeconds=0;e.downloaded=false;out.push_back(e);}
{ Episode e;e.id="nyfiken-1";e.podcastId="nyfiken";e.title="Varför sjunger valarna?";e.publicationDate="2026-11-05";e.audioUrl="https://example.invalid/audio/nyfiken0.mp3";e.mimeType="audio/mpeg";e.durationSeconds=2382;e.positionSeconds=0;e.downloaded=false;out.push_back(e);}
{ Episode e;e.id="nyfiken-2";e.podcastId="nyfiken";e.title="Skogen under våra fötter";e.publicationDate="2026-11-03";e.audioUrl="https://example.invalid/audio/nyfiken1.mp3";e.mimeType="audio/mpeg";e.durationSeconds=1940;e.positionSeconds=0;e.downloaded=false;out.push_back(e);}
{ Episode e;e.id="nyfiken-3";e.podcastId="nyfiken";e.title="En resa till månens baksida";e.publicationDate="2026-11-01";e.audioUrl="https://example.invalid/audio/nyfiken2.mp3";e.mimeType="audio/mpeg";e.durationSeconds=2715;e.positionSeconds=0;e.downloaded=false;out.push_back(e);}
{ Episode e;e.id="sondag-1";e.podcastId="sondag";e.title="Doften av nybakat";e.publicationDate="2026-11-05";e.audioUrl="https://example.invalid/audio/sondag0.mp3";e.mimeType="audio/mpeg";e.durationSeconds=2382;e.positionSeconds=0;e.downloaded=false;out.push_back(e);}
{ Episode e;e.id="sondag-2";e.podcastId="sondag";e.title="Ett recept som går i arv";e.publicationDate="2026-11-03";e.audioUrl="https://example.invalid/audio/sondag1.mp3";e.mimeType="audio/mpeg";e.durationSeconds=1940;e.positionSeconds=0;e.downloaded=false;out.push_back(e);}
{ Episode e;e.id="sondag-3";e.podcastId="sondag";e.title="Runt samma bord";e.publicationDate="2026-11-01";e.audioUrl="https://example.invalid/audio/sondag2.mp3";e.mimeType="audio/mpeg";e.durationSeconds=2715;e.positionSeconds=0;e.downloaded=false;out.push_back(e);}
return out; }
}
