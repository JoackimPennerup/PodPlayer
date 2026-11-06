#pragma once
#include "Interfaces.hpp"
#include <algorithm>
#include <utility>

namespace pod {
// Deterministic audio simulator, never selected silently for real hardware playback.
class MockAudioPlayer final : public AudioPlayer {
    PlaybackState state_ = PlaybackState::Stopped;
    uint32_t position_ = 0, duration_ = 0;
    uint8_t volume_ = 65;
    bool seekable_ = true;
public:
    explicit MockAudioPlayer(uint32_t duration = 2382, bool seekable = true)
        : duration_(duration), seekable_(seekable) {}
    Result play(const std::string& url) override {
        if (url.empty()) return Result::InvalidArgument;
        position_ = 0; state_ = PlaybackState::Playing; return Result::Ok;
    }
    Result pause() override {
        if (state_ != PlaybackState::Playing) return Result::InvalidArgument;
        state_ = PlaybackState::Paused; return Result::Ok;
    }
    Result resume() override {
        if (state_ != PlaybackState::Paused) return Result::InvalidArgument;
        state_ = PlaybackState::Playing; return Result::Ok;
    }
    Result stop() override { state_ = PlaybackState::Stopped; position_ = 0; return Result::Ok; }
    Result seekRelativeSeconds(int32_t delta) override {
        if (!seekable_) return Result::Unsupported;
        if (state_ == PlaybackState::Stopped || state_ == PlaybackState::Error) return Result::InvalidArgument;
        position_ = static_cast<uint32_t>(std::clamp<int64_t>(int64_t(position_)+delta, 0, duration_));
        if (position_ == duration_) state_ = PlaybackState::Completed;
        else if (state_ == PlaybackState::Completed) state_ = PlaybackState::Paused;
        return Result::Ok;
    }
    Result setVolume(uint8_t value) override {
        if (value > 100) return Result::InvalidArgument;
        volume_ = value; return Result::Ok;
    }
    uint8_t volume() const { return volume_; }
    PlaybackState getPlaybackState() const override { return state_; }
    uint32_t getPlaybackPosition() const override { return position_; }
    void tick(uint32_t seconds) {
        if (state_ != PlaybackState::Playing) return;
        position_ = static_cast<uint32_t>(std::min<uint64_t>(uint64_t(position_)+seconds, duration_));
        if (position_ == duration_) state_ = PlaybackState::Completed;
    }
};

class PendingAudioPlayer final : public AudioPlayer {
public:
    Result play(const std::string&) override { return Result::Unavailable; }
    Result pause() override { return Result::Unavailable; }
    Result resume() override { return Result::Unavailable; }
    Result stop() override { return Result::Unavailable; }
    Result seekRelativeSeconds(int32_t) override { return Result::Unsupported; }
    Result setVolume(uint8_t) override { return Result::Unavailable; }
    PlaybackState getPlaybackState() const override { return PlaybackState::Error; }
    uint32_t getPlaybackPosition() const override { return 0; }
};

// Poll on the application worker, not from the LVGL callback.
class ProgressPersistence {
    PlaybackStateStore& store_;
    uint64_t lastSavedMs_ = 0;
    bool dirty_ = false;
public:
    static constexpr uint64_t IntervalMs = 30000;
    explicit ProgressPersistence(PlaybackStateStore& store) : store_(store) {}
    void markDirty() { dirty_ = true; }
    Result flush(const SavedState& state, uint64_t nowMs, bool importantTransition = false) {
        if (!dirty_ || (!importantTransition && nowMs-lastSavedMs_ < IntervalMs)) return Result::Ok;
        auto result = store_.save(state);
        if (result == Result::Ok) { dirty_ = false; lastSavedMs_ = nowMs; }
        return result;
    }
};

class MemoryStateStore final : public PlaybackStateStore {
    SavedState state_;
    bool populated_ = false;
public:
    size_t writes = 0;
    Result load(SavedState& output) override {
        if (!populated_) return Result::Unavailable;
        output = state_; return Result::Ok;
    }
    Result save(const SavedState& input) override {
        state_ = input; populated_ = true; ++writes; return Result::Ok;
    }
};
} // namespace pod
