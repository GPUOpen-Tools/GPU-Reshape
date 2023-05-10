#pragma once

#include <Message/MessageStream.h>

struct MessageStreamHeaderProtocol {
    static constexpr uint64_t kMagic = 'GBVS';

    /// Magic header for validation
    uint64_t magic = kMagic;

    /// Schema of the stream
    MessageSchema schema;

    /// Version of the stream
    uint32_t versionID;
    uint32_t : 32;

    /// Size of the succeeding stream
    uint64_t size{};
};

static_assert(sizeof(MessageStreamHeaderProtocol) == 32, "Unexpected message stream protocol size");
