#pragma once
#include "Interfaces.hpp"
#include "StateCodec.hpp"

namespace pod {
class BlobStateStore final : public PlaybackStateStore {
    BlobStorage& storage_;
public:
    explicit BlobStateStore(BlobStorage& storage):storage_(storage) {}
    Result load(SavedState& state) override {
        std::vector<uint8_t> blob; const auto result=storage_.read(blob);
        return result==Result::Ok ? StateCodec::decode(blob,state) : result;
    }
    Result save(const SavedState& state) override {
        std::vector<uint8_t> blob; const auto result=StateCodec::encode(state,blob);
        return result==Result::Ok ? storage_.writeAtomically(blob) : result;
    }
};
} // namespace pod
