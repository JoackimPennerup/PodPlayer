#pragma once
#include "Model.hpp"
#include <cstddef>

namespace pod {
// Incremental, bounded RSS subset. No DTDs, external entities or network access.
// Feed byte chunks in order, then call finish. A failed parse must not be published.
class RssFeedParser {
public:
    static constexpr size_t MaxFeedBytes = 512 * 1024, MaxTokenBytes = 8192;
    static constexpr size_t MaxEpisodes = 40, MaxDepth = 24, MaxFieldBytes = 2048;
    explicit RssFeedParser(Podcast source);
    Result feed(const char* bytes, size_t length);
    Result finish();
    const Podcast& podcast() const { return podcast_; }
    const std::vector<Episode>& episodes() const { return episodes_; }
    static bool parseDuration(const std::string&, uint32_t& seconds);
private:
    Podcast podcast_;
    std::vector<Episode> episodes_;
    Episode item_;
    std::vector<std::string> stack_;
    std::string pending_, text_;
    size_t total_ = 0, itemNumber_ = 0;
    bool inItem_ = false, sawRss_ = false, sawChannel_ = false, closedRss_ = false;
    Result status_ = Result::Ok;
    Result process();
    Result tag(const std::string&);
    Result content(const std::string&, bool cdata = false);
    Result close(const std::string&);
};
} // namespace pod
