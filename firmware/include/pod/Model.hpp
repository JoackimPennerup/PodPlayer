#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace pod {
struct Podcast {
    std::string id, title, feedUrl, artworkUrl;
};
struct Episode {
    std::string id, podcastId, title, publicationDate, artworkUrl;
    std::string audioUrl, mimeType;
    uint64_t enclosureBytes = 0;
    uint32_t durationSeconds = 0, positionSeconds = 0;
    bool completed = false, downloaded = false;
};
enum class PlaybackState { Stopped, Playing, Paused, Buffering, Error, Completed };
enum class Result { Ok, Unsupported, InvalidArgument, Offline, IoError, Unavailable, InvalidData, LimitExceeded };
struct SavedState {
    std::vector<Podcast> subscriptions;
    std::string episodeId;
    uint32_t positionSeconds = 0;
    uint8_t volume = 65;
    bool resumeEnabled = true;
};
enum class WifiState { Unconfigured, Connecting, Connected, Disconnected };
} // namespace pod
