#include "pod/Application.hpp"
#include "pod/BlobStateStore.hpp"
#include "pod/Board.hpp"
#include "pod/RssFeedParser.hpp"
#include "pod/Services.hpp"
#include "pod/Ui.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>

int checks=0;
#define CHECK(expression) do { ++checks; if(!(expression)) { std::cerr << __FILE__ << ':' << __LINE__ << " failed: " #expression "\n"; std::exit(1); } } while(false)
using namespace pod;
Podcast source{"demo","Demo","https://example.invalid/feed.xml",""};
Result parse(const std::string& xml,RssFeedParser& parser,size_t chunk=7) {
    for(size_t offset=0;offset<xml.size();offset+=chunk) {
        auto result=parser.feed(xml.data()+offset,std::min(chunk,xml.size()-offset));
        if(result!=Result::Ok) return result;
    }
    return parser.finish();
}
std::string rss(const std::string& item) { return "<rss><channel>"+item+"</channel></rss>"; }
std::string item(const std::string& extra="") { return "<item><title>Same title</title><enclosure url='https://example.invalid/a.mp3' type='audio/mpeg'/>"+extra+"</item>"; }
class FixtureNetwork:public NetworkClient {
public:
    std::string data;
    Result result=Result::Ok;
    Result get(const std::string&,size_t limit,const std::function<bool(const char*,size_t)>& sink) override {
        if(result!=Result::Ok) return result;
        if(data.size()>limit) return Result::LimitExceeded;
        for(size_t i=0;i<data.size();i+=13) if(!sink(data.data()+i,std::min(size_t(13),data.size()-i))) return Result::InvalidData;
        return Result::Ok;
    }
};
class TestBlob:public BlobStorage {
public:
    std::vector<uint8_t> bytes;
    Result read(std::vector<uint8_t>& out) override { if(bytes.empty()) return Result::Unavailable; out=bytes; return Result::Ok; }
    Result writeAtomically(const std::vector<uint8_t>& data) override { bytes=data; return Result::Ok; }
};
class TestWifi:public WifiAdapter {
public:
    WifiState current=WifiState::Disconnected; unsigned attempts=0;
    WifiState state() const override { return current; }
    Result reconnect() override { ++attempts; return Result::Offline; }
};
void testRss(const std::string& fixture) {
    for(size_t chunk:{size_t(1),size_t(7),size_t(4096)}) {
        RssFeedParser p(source); CHECK(parse(fixture,p,chunk)==Result::Ok);
        CHECK(p.podcast().title=="Mellan raderna & andra berättelser");
        CHECK(p.podcast().artworkUrl=="https://example.invalid/art.jpg");
        CHECK(p.episodes().size()==2);
        CHECK(p.episodes()[0].id=="urn:demo:episode:001");
        CHECK(p.episodes()[0].title=="När staden somnar");
        CHECK(p.episodes()[0].durationSeconds==2382);
        CHECK(p.episodes()[0].audioUrl=="https://example.invalid/episode.mp3?a=1&b=2");
        CHECK(p.episodes()[0].mimeType=="audio/mpeg");
        CHECK(p.episodes()[0].enclosureBytes==12345678);
        CHECK(p.episodes()[0].artworkUrl=="https://example.invalid/episode.jpg");
        CHECK(p.episodes()[1].title=="Ett brev från förr");
        CHECK(p.episodes()[1].durationSeconds==1940);
        CHECK(p.episodes()[1].artworkUrl==p.podcast().artworkUrl);
    }
    uint32_t seconds=999;
    CHECK(RssFeedParser::parseDuration("1:02:03",seconds)&&seconds==3723);
    CHECK(RssFeedParser::parseDuration("39:42",seconds)&&seconds==2382);
    CHECK(!RssFeedParser::parseDuration("1:60",seconds));
    CHECK(!RssFeedParser::parseDuration("-2",seconds));
    CHECK(!RssFeedParser::parseDuration("999999999999999999999999",seconds));
    CHECK(!RssFeedParser::parseDuration("1:2:3:4",seconds));
    RssFeedParser missing(source); CHECK(parse(rss(item()),missing)==Result::Ok);
    CHECK(missing.episodes()[0].id.rfind("url:",0)==0);
    RssFeedParser again(source); CHECK(parse(rss(item()),again)==Result::Ok);
    CHECK(again.episodes()[0].id==missing.episodes()[0].id);
    RssFeedParser sameTitles(source); CHECK(parse(rss(item("<guid>a</guid>")+item("<guid>b</guid>")),sameTitles)==Result::Ok); CHECK(sameTitles.episodes().size()==2);
    RssFeedParser duplicates(source); CHECK(parse(rss(item("<guid>a</guid>")+item("<guid>a</guid>")),duplicates)==Result::Ok); CHECK(duplicates.episodes().size()==1);
    RssFeedParser empty(source); CHECK(parse(rss(""),empty)==Result::Ok); CHECK(empty.episodes().empty());
    for(const std::string bad:{"<rss><channel>","<rss><channel></rss>","<!DOCTYPE rss SYSTEM 'file:///etc/passwd'><rss><channel/></rss>","<rss><channel><title>&external;</title></channel></rss>","<rss><channel/></rss><rss/>","< >"}) {
        RssFeedParser p(source); CHECK(parse(bad,p)!=Result::Ok);
    }
    RssFeedParser huge(source); CHECK(parse(rss("<title>"+std::string(9000,'x')+"</title>"),huge)==Result::LimitExceeded);
    RssFeedParser largeFeed(source); std::string bytes(RssFeedParser::MaxFeedBytes+1,' '); CHECK(largeFeed.feed(bytes.data(),bytes.size())==Result::LimitExceeded);
    RssFeedParser maxItems(source); std::string many;
    for(unsigned i=0;i<41;++i) many+=item("<guid>"+std::to_string(i)+"</guid>");
    CHECK(parse(rss(many),maxItems)==Result::LimitExceeded);
    RssFeedParser depth(source); CHECK(parse(rss(std::string(0,'x')+"<a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a><a>"),depth)==Result::LimitExceeded);
    FixtureNetwork network; network.data=fixture; FeedRepository repository(network,{source});
    std::vector<Episode> episodes; CHECK(repository.episodes(source,episodes)==Result::Ok); CHECK(episodes.size()==2);
    network.data="<rss>broken"; CHECK(repository.episodes(source,episodes)==Result::InvalidData); CHECK(episodes.size()==2);
    network.result=Result::Offline; CHECK(repository.episodes(source,episodes)==Result::Offline);
}
void testPlayback() {
    MockAudioPlayer audio(30);
    CHECK(audio.resume()==Result::InvalidArgument);
    CHECK(audio.play("")==Result::InvalidArgument);
    CHECK(audio.play("demo")==Result::Ok);
    CHECK(audio.getPlaybackState()==PlaybackState::Playing);
    audio.tick(4); CHECK(audio.getPlaybackPosition()==4);
    CHECK(audio.pause()==Result::Ok); audio.tick(10); CHECK(audio.getPlaybackPosition()==4);
    CHECK(audio.seekRelativeSeconds(-100)==Result::Ok); CHECK(audio.getPlaybackPosition()==0);
    CHECK(audio.seekRelativeSeconds(INT32_MAX)==Result::Ok); CHECK(audio.getPlaybackPosition()==30); CHECK(audio.getPlaybackState()==PlaybackState::Completed);
    CHECK(audio.seekRelativeSeconds(-10)==Result::Ok); CHECK(audio.getPlaybackState()==PlaybackState::Paused);
    CHECK(audio.resume()==Result::Ok); audio.tick(UINT32_MAX); CHECK(audio.getPlaybackPosition()==30);
    CHECK(audio.setVolume(100)==Result::Ok); CHECK(audio.volume()==100); CHECK(audio.setVolume(101)==Result::InvalidArgument);
    CHECK(audio.stop()==Result::Ok); CHECK(audio.getPlaybackPosition()==0);
    MockAudioPlayer unseekable(30,false); CHECK(unseekable.play("demo")==Result::Ok); unseekable.tick(5); CHECK(unseekable.seekRelativeSeconds(10)==Result::Unsupported); CHECK(unseekable.getPlaybackPosition()==5);
    MemoryStateStore store; SavedState state; state.episodeId="stable-id"; state.positionSeconds=22;
    ProgressPersistence writer(store); writer.markDirty(); CHECK(writer.flush(state,29999)==Result::Ok); CHECK(store.writes==0);
    CHECK(writer.flush(state,30000)==Result::Ok); CHECK(store.writes==1);
    writer.markDirty(); CHECK(writer.flush(state,30001,true)==Result::Ok); CHECK(store.writes==2);
    CHECK(writer.flush(state,60000)==Result::Ok); CHECK(store.writes==2);
    SavedState restored; CHECK(store.load(restored)==Result::Ok); CHECK(restored.episodeId==state.episodeId); CHECK(restored.positionSeconds==22);
    Episode e; e.id="episode"; e.audioUrl="demo"; e.durationSeconds=30;
    MockAudioPlayer appAudio(30); MemoryStateStore appStore; Application app(appAudio,appStore);
    CHECK(app.process({CommandType::Play,0,e},0)==Result::Ok); appAudio.tick(12); CHECK(app.tick(1000)==Result::Ok);
    CHECK(app.process({CommandType::Toggle},2000)==Result::Ok);
    MockAudioPlayer rebootAudio(30); Application reboot(rebootAudio,appStore);
    CHECK(reboot.process({CommandType::Play,0,e},0)==Result::Ok); CHECK(rebootAudio.getPlaybackPosition()==12);
    CHECK(reboot.process({CommandType::Volume,45},1)==Result::Ok); CHECK(reboot.saved().volume==45);
    CHECK(reboot.process({CommandType::Stop},2)==Result::Ok); CHECK(reboot.saved().positionSeconds==12);
    PendingAudioPlayer pending; CHECK(pending.play("demo")==Result::Unavailable); CHECK(pending.seekRelativeSeconds(10)==Result::Unsupported);
}
void testState() {
    SavedState state; state.subscriptions={source}; state.episodeId="urn:1"; state.positionSeconds=456; state.volume=32; state.resumeEnabled=false;
    std::vector<uint8_t> bytes; CHECK(StateCodec::encode(state,bytes)==Result::Ok);
    SavedState output; CHECK(StateCodec::decode(bytes,output)==Result::Ok);
    CHECK(output.episodeId==state.episodeId); CHECK(output.positionSeconds==456); CHECK(output.volume==32); CHECK(!output.resumeEnabled); CHECK(output.subscriptions[0].feedUrl==source.feedUrl);
    for(size_t i=0;i<bytes.size();++i) { auto broken=bytes; broken[i]^=1; CHECK(StateCodec::decode(broken,output)==Result::InvalidData); }
    for(size_t i=0;i<bytes.size();++i) { std::vector<uint8_t> cut(bytes.begin(),bytes.begin()+i); CHECK(StateCodec::decode(cut,output)==Result::InvalidData); }
    state.volume=101; CHECK(StateCodec::encode(state,bytes)==Result::InvalidArgument);
    TestBlob blob; BlobStateStore store(blob); CHECK(store.load(output)==Result::Unavailable);
    state.volume=65; CHECK(store.save(state)==Result::Ok); CHECK(store.load(output)==Result::Ok); CHECK(output.episodeId==state.episodeId);
}
void testBoardAndUi() {
    using namespace pod::board;
    auto v1=selectTouch([](uint8_t address){return address==0x1a;}); CHECK(v1.revision==TouchRevision::V1_Cst328); CHECK(v1.detected);
    auto v2=selectTouch([](uint8_t address){return address==0x58;}); CHECK(v2.revision==TouchRevision::V2_Cst3530); CHECK(v2.detected);
    auto both=selectTouch([](uint8_t){return true;}); CHECK(both.result==Result::InvalidData);
    auto none=selectTouch([](uint8_t){return false;}); CHECK(none.revision==DefaultTouchRevision); CHECK(!none.detected);
    unsigned probes=0; auto configured=selectTouch([&](uint8_t){++probes;return true;},false); CHECK(probes==0); CHECK(configured.revision==DefaultTouchRevision);
    Cst328TouchDriver touch1; Cst3530TouchDriver touch2; CHECK(touch1.begin()==Result::Unavailable); CHECK(touch2.begin()==Result::Unavailable);
    UiSnapshot snapshot; snapshot.screen=Screen::Player; snapshot.episode.title="När staden somnar"; snapshot.episode.durationSeconds=2382;
    unsigned buttons=0;
    for(const auto& widget:layout(snapshot)) {
        CHECK(widget.x>=0 && widget.y>=0 && widget.x+widget.width<=240 && widget.y+widget.height<=320);
        if(widget.kind==WidgetKind::Button) { CHECK(widget.width>=44 && widget.height>=44); ++buttons; }
    }
    CHECK(buttons==5);
    TestWifi wifi; WifiReconnector reconnect(wifi); CHECK(reconnect.tick(0)==Result::Offline); CHECK(wifi.attempts==1);
    CHECK(reconnect.tick(999)==Result::Ok); CHECK(wifi.attempts==1); CHECK(reconnect.tick(1000)==Result::Offline); CHECK(wifi.attempts==2);
    wifi.current=WifiState::Connected; CHECK(reconnect.tick(1001)==Result::Ok);
    CHECK(ItunesSearchService::requestUrl("mat & öl").find("mat%20%26%20%C3%B6l")!=std::string::npos);
    FixtureSearchService search({source,{"no-feed","Skip me","",""}}); std::vector<Podcast> results;
    CHECK(search.search("",results)==Result::Ok); CHECK(results.size()==1);
}
int main(int argc,char** argv) {
    CHECK(argc==2); std::ifstream input(argv[1]); CHECK(input.good());
    const std::string fixture((std::istreambuf_iterator<char>(input)),{});
    testRss(fixture); testPlayback(); testState(); testBoardAndUi();
    std::cout << "PASS: " << checks << " native checks (RSS, playback, persistence, touch selection, UI, services)\n";
}
