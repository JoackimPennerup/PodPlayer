#pragma once
#include "Interfaces.hpp"
#include "RssFeedParser.hpp"
#include <algorithm>

namespace pod {
class FeedRepository final : public PodcastRepository {
    NetworkClient& network_;
    std::vector<Podcast> favorites_;
public:
    FeedRepository(NetworkClient& network, std::vector<Podcast> favorites)
        : network_(network), favorites_(std::move(favorites)) {}
    std::vector<Podcast> subscriptions() const override { return favorites_; }
    Result episodes(const Podcast& source, std::vector<Episode>& output) override {
        RssFeedParser parser(source);
        Result parseResult=Result::Ok;
        const auto result=network_.get(source.feedUrl,RssFeedParser::MaxFeedBytes,
            [&](const char* bytes,size_t count) {
                parseResult=parser.feed(bytes,count); return parseResult==Result::Ok;
            });
        if (parseResult!=Result::Ok) return parseResult;
        if (result!=Result::Ok) return result;
        const auto finished=parser.finish();
        if (finished!=Result::Ok) return finished;
        output=parser.episodes(); return Result::Ok;
    }
};
class FixtureSearchService final : public PodcastSearchService {
    std::vector<Podcast> data_;
public:
    explicit FixtureSearchService(std::vector<Podcast> data) : data_(std::move(data)) {}
    Result search(const std::string& query,std::vector<Podcast>& output) override {
        output.clear();
        for (const auto& p:data_) if (!p.feedUrl.empty() && (query.empty() || p.title.find(query)!=std::string::npos)) output.push_back(p);
        return Result::Ok;
    }
};
class ItunesSearchService final : public PodcastSearchService {
public:
    static std::string requestUrl(const std::string& query) {
        constexpr char hex[]="0123456789ABCDEF"; std::string encoded;
        for (unsigned char c:query) {
            if ((c>='a'&&c<='z') || (c>='A'&&c<='Z') || (c>='0'&&c<='9') || c=='-' || c=='_' || c=='.' || c=='~') encoded+=char(c);
            else { encoded+='%'; encoded+=hex[c>>4]; encoded+=hex[c&15]; }
        }
        return "https://itunes.apple.com/search?term="+encoded+"&media=podcast&entity=podcast&country=se&limit=20";
    }
    Result search(const std::string&,std::vector<Podcast>&) override {
        // JSON and HTTPS adapter await verified local dependencies. Never pretend fixtures are live results.
        return Result::Unavailable;
    }
};
// Call tick() on worker task. Backoff is deterministic for local tests.
class WifiReconnector {
    WifiAdapter& adapter_;
    uint64_t nextAttempt_=0;
    uint32_t delayMs_=1000;
public:
    explicit WifiReconnector(WifiAdapter& adapter):adapter_(adapter) {}
    Result tick(uint64_t nowMs) {
        const auto state=adapter_.state();
        if (state==WifiState::Connected) { delayMs_=1000; nextAttempt_=nowMs; return Result::Ok; }
        if (state==WifiState::Unconfigured) return Result::Unavailable;
        if (state==WifiState::Connecting || nowMs<nextAttempt_) return Result::Ok;
        const auto result=adapter_.reconnect(); nextAttempt_=nowMs+delayMs_;
        delayMs_=std::min<uint32_t>(delayMs_*2,60000); return result;
    }
};
} // namespace pod
