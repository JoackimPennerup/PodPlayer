#pragma once
#include "Model.hpp"
#include <functional>

namespace pod {
class PodcastRepository {
public:
    virtual ~PodcastRepository() = default;
    virtual std::vector<Podcast> subscriptions() const = 0;
    virtual Result episodes(const Podcast&, std::vector<Episode>&) = 0;
};
class PodcastSearchService {
public:
    virtual ~PodcastSearchService() = default;
    virtual Result search(const std::string& query, std::vector<Podcast>& results) = 0;
};
class AudioPlayer {
public:
    virtual ~AudioPlayer() = default;
    virtual Result play(const std::string& audioUrl) = 0;
    virtual Result pause() = 0;
    virtual Result resume() = 0;
    virtual Result stop() = 0;
    virtual Result seekRelativeSeconds(int32_t delta) = 0;
    virtual Result setVolume(uint8_t percent) = 0;
    virtual PlaybackState getPlaybackState() const = 0;
    virtual uint32_t getPlaybackPosition() const = 0;
};
class PlaybackStateStore {
public:
    virtual ~PlaybackStateStore() = default;
    virtual Result load(SavedState&) = 0;
    virtual Result save(const SavedState&) = 0;
};
class DisplayDriver {
public:
    virtual ~DisplayDriver() = default;
    virtual Result begin() = 0;
    // RGB565 pixels; hardware adapter owns DMA completion and LVGL flush acknowledgement.
    virtual Result flush(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* pixels) = 0;
};
struct TouchPoint { uint16_t x = 0, y = 0; bool pressed = false; };
class TouchDriver {
public:
    virtual ~TouchDriver() = default;
    virtual Result begin() = 0;
    virtual Result read(TouchPoint&) = 0;
};
class NetworkClient {
public:
    virtual ~NetworkClient() = default;
    // Runs on worker task. Must verify TLS, bound redirects/bytes/time and propagate sink failure.
    virtual Result get(const std::string& url, size_t maxBytes,
                       const std::function<bool(const char*, size_t)>& sink) = 0;
};
class WifiAdapter {
public:
    virtual ~WifiAdapter() = default;
    virtual WifiState state() const = 0;
    virtual Result reconnect() = 0;
};
} // namespace pod
