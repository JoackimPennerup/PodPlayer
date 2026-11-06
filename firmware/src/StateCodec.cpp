#include "pod/StateCodec.hpp"
#include <array>

namespace pod {
namespace {
uint32_t crc(const uint8_t* bytes,size_t count) {
    uint32_t value=0xffffffff;
    for(size_t i=0;i<count;++i) {
        value^=bytes[i];
        for(int bit=0;bit<8;++bit) value=(value>>1)^(0xedb88320U & (0U-(value&1U)));
    }
    return ~value;
}
void number(std::vector<uint8_t>& out,uint32_t n) {
    for(unsigned i=0;i<4;++i) out.push_back(uint8_t(n>>(i*8)));
}
bool string(std::vector<uint8_t>& out,const std::string& s) {
    if(s.size()>2048 || out.size()+s.size()+4>StateCodec::MaxBytes-4) return false;
    number(out,uint32_t(s.size())); out.insert(out.end(),s.begin(),s.end()); return true;
}
struct Reader {
    const std::vector<uint8_t>& data; size_t at=4;
    bool number(uint32_t& n) {
        if(at+4>data.size()-4) return false;
        n=0; for(unsigned i=0;i<4;++i) n|=uint32_t(data[at++])<<(i*8); return true;
    }
    bool string(std::string& s) {
        uint32_t length=0;
        if(!number(length) || length>2048 || length>data.size()-4-at) return false;
        s.assign(reinterpret_cast<const char*>(data.data()+at),length); at+=length; return true;
    }
};
}
Result StateCodec::encode(const SavedState& state,std::vector<uint8_t>& output) {
    if(state.volume>100 || state.subscriptions.size()>MaxSubscriptions) return Result::InvalidArgument;
    std::vector<uint8_t> out={'P','O','D',1};
    number(out,state.positionSeconds); number(out,state.volume); number(out,state.resumeEnabled);
    if(!string(out,state.episodeId)) return Result::LimitExceeded;
    number(out,uint32_t(state.subscriptions.size()));
    for(const auto& p:state.subscriptions) {
        if(!string(out,p.id)||!string(out,p.title)||!string(out,p.feedUrl)||!string(out,p.artworkUrl)) return Result::LimitExceeded;
    }
    if(out.size()+4>MaxBytes) return Result::LimitExceeded;
    number(out,crc(out.data(),out.size())); output=std::move(out); return Result::Ok;
}
Result StateCodec::decode(const std::vector<uint8_t>& input,SavedState& output) {
    if(input.size()<28 || input.size()>MaxBytes || input[0]!='P' || input[1]!='O' || input[2]!='D' || input[3]!=1) return Result::InvalidData;
    uint32_t expected=0;
    for(unsigned i=0;i<4;++i) expected|=uint32_t(input[input.size()-4+i])<<(i*8);
    if(crc(input.data(),input.size()-4)!=expected) return Result::InvalidData;
    Reader reader{input}; SavedState state; uint32_t volume=0,resume=0,count=0;
    if(!reader.number(state.positionSeconds)||!reader.number(volume)||volume>100||!reader.number(resume)||resume>1||!reader.string(state.episodeId)||!reader.number(count)||count>MaxSubscriptions) return Result::InvalidData;
    state.volume=uint8_t(volume); state.resumeEnabled=resume;
    for(uint32_t i=0;i<count;++i) {
        Podcast p;
        if(!reader.string(p.id)||!reader.string(p.title)||!reader.string(p.feedUrl)||!reader.string(p.artworkUrl)) return Result::InvalidData;
        state.subscriptions.push_back(std::move(p));
    }
    if(reader.at!=input.size()-4) return Result::InvalidData;
    output=std::move(state); return Result::Ok;
}
} // namespace pod
