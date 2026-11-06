#include "pod/RssFeedParser.hpp"
#include <algorithm>
#include <cctype>
#include <limits>
#include <map>

namespace pod {
namespace {
std::string trim(std::string s) {
    auto first = s.find_first_not_of(" \r\n\t");
    return first == std::string::npos ? "" : s.substr(first, s.find_last_not_of(" \r\n\t")-first+1);
}
bool uintValue(const std::string& s, uint64_t& n) {
    if (s.empty()) return false;
    n = 0;
    for (unsigned char c : s) {
        if (c < '0' || c > '9' || n > (std::numeric_limits<uint64_t>::max()-(c-'0'))/10) return false;
        n = n*10+c-'0';
    }
    return true;
}
void utf8(std::string& s, uint32_t n) {
    if (n < 128) s += char(n);
    else if (n < 2048) { s += char(0xc0 | n>>6); s += char(0x80 | (n&63)); }
    else if (n < 65536) { s += char(0xe0 | n>>12); s += char(0x80 | ((n>>6)&63)); s += char(0x80 | (n&63)); }
    else { s += char(0xf0 | n>>18); s += char(0x80 | ((n>>12)&63)); s += char(0x80 | ((n>>6)&63)); s += char(0x80 | (n&63)); }
}
bool decode(const std::string& input, std::string& out) {
    for (size_t i=0; i<input.size(); ++i) {
        if (input[i] != '&') { out += input[i]; continue; }
        const auto end = input.find(';', i+1);
        if (end == std::string::npos || end-i > 16) return false;
        const auto entity = input.substr(i+1, end-i-1);
        if (entity == "amp") out += '&'; else if (entity == "lt") out += '<';
        else if (entity == "gt") out += '>'; else if (entity == "quot") out += '"';
        else if (entity == "apos") out += '\'';
        else if (!entity.empty() && entity[0] == '#') {
            uint32_t n = 0; size_t start = 1; unsigned base = 10;
            if (entity.size()>1 && entity[1]=='x') { start=2; base=16; }
            if (start == entity.size()) return false;
            for (size_t j=start; j<entity.size(); ++j) {
                unsigned char c = entity[j];
                unsigned v = c>='0' && c<='9' ? c-'0' : c>='a' && c<='f' ? c-'a'+10 : c>='A' && c<='F' ? c-'A'+10 : 99;
                if (v>=base || n>(0x10ffff-v)/base) return false;
                n=n*base+v;
            }
            if (!n || (n<32 && n!=9 && n!=10 && n!=13) || (n>=0xd800 && n<=0xdfff)) return false;
            utf8(out,n);
        } else return false;
        i = end;
    }
    return true;
}
bool httpUrl(const std::string& s) {
    return s.rfind("https://",0)==0 || s.rfind("http://",0)==0;
}
std::string stableFallback(const std::string& value) {
    // Defined FNV-1a, unlike implementation-dependent std::hash.
    uint64_t hash=14695981039346656037ULL;
    for (unsigned char c : value) { hash ^= c; hash *= 1099511628211ULL; }
    return "url:" + std::to_string(hash);
}
}
RssFeedParser::RssFeedParser(Podcast source) : podcast_(std::move(source)) {}
bool RssFeedParser::parseDuration(const std::string& value, uint32_t& output) {
    std::string s=trim(value); uint64_t total=0; size_t start=0, parts=0;
    while (true) {
        auto end=s.find(':', start); uint64_t n=0;
        if (!uintValue(s.substr(start,end==std::string::npos ? end : end-start),n)) return false;
        if (++parts>3 || (parts>1 && n>=60) || n>UINT32_MAX || total>(UINT32_MAX-n)/60) return false;
        total=total*60+n;
        if (end==std::string::npos) break;
        start=end+1;
    }
    output=static_cast<uint32_t>(total); return true;
}
Result RssFeedParser::feed(const char* bytes, size_t length) {
    if (status_ != Result::Ok) return status_;
    if ((!bytes && length) || length > MaxFeedBytes-total_) return status_=Result::LimitExceeded;
    total_ += length;
    // Append in small slices so even one huge feed() call cannot allocate an unbounded token.
    for (size_t i=0;i<length;++i) {
        pending_ += bytes[i];
        if (pending_.size()>MaxTokenBytes) return status_=Result::LimitExceeded;
        if (bytes[i]=='>' || (pending_[0]!='<' && bytes[i]=='<')) {
            status_=process(); if (status_!=Result::Ok) return status_;
        }
    }
    return status_;
}
Result RssFeedParser::content(const std::string& raw, bool cdata) {
    if (stack_.empty()) return trim(raw).empty() ? Result::Ok : Result::InvalidData;
    std::string decoded;
    if (cdata) decoded=raw;
    else if (!decode(raw,decoded)) return Result::InvalidData;
    // Only accumulate fields we consume; ignored text is still token-bounded.
    const auto& name=stack_.back();
    if (name=="title" || name=="guid" || name=="pubDate" || name=="itunes:duration" || name=="url") {
        if (decoded.size()>MaxFieldBytes-text_.size()) return Result::LimitExceeded;
        text_ += decoded;
    }
    return Result::Ok;
}
Result RssFeedParser::process() {
    while (!pending_.empty()) {
        if (pending_[0]!='<') {
            auto end=pending_.find('<');
            if (end==std::string::npos) return Result::Ok;
            auto r=content(pending_.substr(0,end)); if (r!=Result::Ok) return r;
            pending_.erase(0,end); continue;
        }
        if (pending_.rfind("<!--",0)==0) {
            auto end=pending_.find("-->"); if (end==std::string::npos) return Result::Ok;
            pending_.erase(0,end+3); continue;
        }
        if (pending_.rfind("<![CDATA[",0)==0) {
            auto end=pending_.find("]]>"); if (end==std::string::npos) return Result::Ok;
            auto r=content(pending_.substr(9,end-9),true); if (r!=Result::Ok) return r;
            pending_.erase(0,end+3); continue;
        }
        // '>' inside a quoted enclosure URL is not a tag boundary.
        char quote=0; size_t end=1;
        for (;end<pending_.size();++end) {
            char c=pending_[end];
            if (quote) { if (c==quote) quote=0; }
            else if (c=='\'' || c=='"') quote=c;
            else if (c=='>') break;
        }
        if (end==pending_.size()) return Result::Ok;
        auto raw=pending_.substr(1,end-1); pending_.erase(0,end+1);
        if (raw.rfind("?xml",0)==0 && !sawRss_ && raw.back()=='?') continue;
        if (raw.empty() || raw[0]=='!' || raw[0]=='?') return Result::InvalidData;
        auto r=tag(raw); if (r!=Result::Ok) return r;
    }
    return Result::Ok;
}
Result RssFeedParser::tag(const std::string& input) {
    auto raw=trim(input);
    if (raw.empty()) return Result::InvalidData;
    if (raw[0]=='/') return close(trim(raw.substr(1)));
    bool self=raw.back()=='/'; if (self) raw=trim(raw.substr(0,raw.size()-1));
    size_t i=0;
    while (i<raw.size() && !std::isspace(static_cast<unsigned char>(raw[i]))) ++i;
    auto name=raw.substr(0,i);
    if (name.empty() || name.size()>80) return Result::InvalidData;
    std::map<std::string,std::string> attrs;
    while (i<raw.size()) {
        while (i<raw.size() && std::isspace(static_cast<unsigned char>(raw[i]))) ++i;
        if (i==raw.size()) break;
        auto begin=i;
        while (i<raw.size() && raw[i]!='=' && !std::isspace(static_cast<unsigned char>(raw[i]))) ++i;
        auto key=raw.substr(begin,i-begin);
        while (i<raw.size() && std::isspace(static_cast<unsigned char>(raw[i]))) ++i;
        if (key.empty() || i==raw.size() || raw[i++]!='=') return Result::InvalidData;
        while (i<raw.size() && std::isspace(static_cast<unsigned char>(raw[i]))) ++i;
        if (i==raw.size() || (raw[i]!='\'' && raw[i]!='"')) return Result::InvalidData;
        char quote=raw[i++]; auto end=raw.find(quote,i);
        if (end==std::string::npos || end-i>MaxFieldBytes || attrs.size()>=24 || attrs.count(key)) return Result::InvalidData;
        std::string value; if (!decode(raw.substr(i,end-i),value)) return Result::InvalidData;
        attrs.emplace(key,std::move(value)); i=end+1;
    }
    if (stack_.size()>=MaxDepth) return Result::LimitExceeded;
    if (stack_.empty()) { if (name!="rss" || sawRss_ || closedRss_) return Result::InvalidData; sawRss_=true; }
    const auto parent=stack_.empty() ? "" : stack_.back();
    if (name=="channel" && parent=="rss") sawChannel_=true;
    if (name=="item" && parent=="channel") { item_={}; item_.podcastId=podcast_.id; inItem_=true; ++itemNumber_; }
    if (name=="enclosure" && parent=="item") {
        if (httpUrl(attrs["url"])) item_.audioUrl=attrs["url"];
        item_.mimeType=attrs["type"]; uint64_t n=0;
        if (uintValue(attrs["length"],n)) item_.enclosureBytes=n;
    }
    if (name=="itunes:image" && httpUrl(attrs["href"])) {
        if (parent=="item") item_.artworkUrl=attrs["href"];
        if (parent=="channel") podcast_.artworkUrl=attrs["href"];
    }
    stack_.push_back(name); text_.clear();
    return self ? close(name) : Result::Ok;
}
Result RssFeedParser::close(const std::string& name) {
    if (stack_.empty() || stack_.back()!=name) return Result::InvalidData;
    std::string value=trim(text_); text_.clear();
    const auto parent=stack_.size()>1 ? stack_[stack_.size()-2] : "";
    if (parent=="item" && inItem_) {
        if (name=="title") item_.title=value;
        else if (name=="guid") item_.id=value;
        else if (name=="pubDate") item_.publicationDate=value;
        else if (name=="itunes:duration") parseDuration(value,item_.durationSeconds);
    }
    if (name=="title" && parent=="channel") podcast_.title=value;
    if (name=="url" && parent=="image" && !inItem_ && httpUrl(value)) podcast_.artworkUrl=value;
    if (name=="item" && parent=="channel") {
        if (!item_.audioUrl.empty()) {
            if (item_.id.empty()) item_.id=stableFallback(item_.audioUrl);
            if (item_.title.empty()) item_.title="Untitled episode";
            if (item_.artworkUrl.empty()) item_.artworkUrl=podcast_.artworkUrl;
            // GUID uniqueness is scoped to a podcast; repeat GUIDs are ignored.
            if (std::none_of(episodes_.begin(),episodes_.end(),[&](const Episode& e){return e.id==item_.id;})) {
                if (episodes_.size()>=MaxEpisodes) return Result::LimitExceeded;
                episodes_.push_back(std::move(item_));
            }
        }
        inItem_=false;
    }
    if (name=="rss") closedRss_=true;
    stack_.pop_back(); return Result::Ok;
}
Result RssFeedParser::finish() {
    if (status_!=Result::Ok) return status_;
    status_=process();
    if (status_!=Result::Ok) return status_;
    if (!trim(pending_).empty() || !stack_.empty() || !sawRss_ || !sawChannel_ || !closedRss_) return status_=Result::InvalidData;
    return Result::Ok;
}
} // namespace pod
