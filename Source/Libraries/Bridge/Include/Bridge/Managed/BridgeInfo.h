#pragma once

using namespace System;

namespace Bridge::CLR {
    public ref struct BridgeInfo {
    public:
        /// Total number of bytes written
        UInt64 bytesWritten{0};

        /// Total number of bytes read
        UInt64 bytesRead{0};
    };
}
