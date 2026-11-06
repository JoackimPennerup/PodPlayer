#pragma once
#include "Model.hpp"

namespace pod {
// Versioned, bounded record for an eventual NVS blob. CRC detects corruption, not tampering.
// No Wi-Fi credentials are stored in this record.
class StateCodec {
public:
    static constexpr size_t MaxBytes=16384, MaxSubscriptions=20;
    static Result encode(const SavedState&,std::vector<uint8_t>&);
    static Result decode(const std::vector<uint8_t>&,SavedState&);
};
class BlobStorage {
public:
    virtual ~BlobStorage() = default;
    virtual Result read(std::vector<uint8_t>&)=0;
    virtual Result writeAtomically(const std::vector<uint8_t>&)=0;
};
} // namespace pod
