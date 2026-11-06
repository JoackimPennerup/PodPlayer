#pragma once
#include "Playback.hpp"
#include <array>

namespace pod {
enum class CommandType { Play, Toggle, Stop, Seek, Volume };
struct Command { CommandType type; int32_t value = 0; Episode episode; };
// Use from a single application worker. UI task communicates through a FreeRTOS queue
// in the future platform adapter; it must not call process() directly.
class Application {
    AudioPlayer& audio_;
    ProgressPersistence persistence_;
    SavedState saved_;
    Episode selected_;
public:
    Result lastResult = Result::Ok;
    Application(AudioPlayer& audio, PlaybackStateStore& store):audio_(audio),persistence_(store) {
        SavedState restored;
        const auto loaded=store.load(restored);
        if (loaded==Result::Ok) saved_=std::move(restored);
        const auto volumeResult=audio_.setVolume(saved_.volume);
        lastResult=(loaded!=Result::Ok && loaded!=Result::Unavailable) ? loaded : volumeResult;
    }
    const SavedState& saved() const { return saved_; }
    const Episode& selected() const { return selected_; }
    Result process(const Command& command, uint64_t nowMs) {
        Result result=Result::InvalidArgument;
        if (command.type==CommandType::Play) {
            // Save the old episode before opening another source.
            if (!selected_.id.empty()) {
                saved_.positionSeconds=audio_.getPlaybackPosition(); persistence_.markDirty();
                result=persistence_.flush(saved_,nowMs,true); if(result!=Result::Ok) return lastResult=result;
            }
            const bool restore=saved_.resumeEnabled && command.episode.id==saved_.episodeId;
            const uint32_t position=restore ? saved_.positionSeconds : command.episode.positionSeconds;
            result=audio_.play(command.episode.audioUrl);
            if(result==Result::Ok) {
                selected_=command.episode; saved_.episodeId=selected_.id;
                if(saved_.resumeEnabled && position) {
                    if(position>INT32_MAX) result=Result::Unsupported;
                    else result=audio_.seekRelativeSeconds(int32_t(position));
                    // Unsupported seek is returned to caller, never presented as resumed.
                }
                saved_.positionSeconds=audio_.getPlaybackPosition(); persistence_.markDirty();
            }
        } else if(command.type==CommandType::Toggle) {
            result=audio_.getPlaybackState()==PlaybackState::Playing ? audio_.pause() : audio_.resume();
        } else if(command.type==CommandType::Seek) result=audio_.seekRelativeSeconds(command.value);
        else if(command.type==CommandType::Volume) {
            if(command.value>=0 && command.value<=100) {
                result=audio_.setVolume(uint8_t(command.value));
                if(result==Result::Ok) saved_.volume=uint8_t(command.value);
            }
        } else if(command.type==CommandType::Stop) {
            saved_.positionSeconds=audio_.getPlaybackPosition(); persistence_.markDirty();
            result=persistence_.flush(saved_,nowMs,true);
            if(result==Result::Ok) result=audio_.stop();
            return lastResult=result;
        }
        if(result==Result::Ok) {
            if(!selected_.id.empty()) saved_.positionSeconds=audio_.getPlaybackPosition();
            persistence_.markDirty();
            result=persistence_.flush(saved_,nowMs,true);
        }
        return lastResult=result;
    }
    Result tick(uint64_t nowMs) {
        if(audio_.getPlaybackState()==PlaybackState::Stopped) return Result::Ok;
        const auto position=audio_.getPlaybackPosition();
        if(saved_.positionSeconds!=position) { saved_.positionSeconds=position; persistence_.markDirty(); }
        return persistence_.flush(saved_,nowMs,audio_.getPlaybackState()==PlaybackState::Completed);
    }
};
} // namespace pod
