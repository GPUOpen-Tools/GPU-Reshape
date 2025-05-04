#pragma once

// Backend
#include <Backend/IL/Format.h>
#include <Backend/IL/Type.h>

// Common
#include <Common/Enum.h>

// Std
#include <cstdint>

enum class BreakpointFlag {
    None = 0,

    /// Allow floating point image compression to 8-8-8-8 texels
    AllowImageFPUNorm8888Compression = BIT(0)
};

enum class BreakpointCompression {
    None,

    /// Using fp 8-8-8-8 compression
    FPUnorm8888
};

BIT_SET(BreakpointFlag);

enum class BreakpointDataOrder {
    None,

    /// Statically known ordering
    /// Each data piece doesn't need to encode its location, it's known implicitly
    Static,

    /// Dynamic ordering
    Dynamic
};

struct BreakpointDynamicHeader {
    /// Thread indices that exported the data
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

struct BreakpointDataHostLayout {
    /// Texel format, None if structural
    Backend::IL::Format format{Backend::IL::Format::None};

    /// If format is None, represents the structural type
    const Backend::IL::Type* type{nullptr};

    /// Compression type
    BreakpointCompression compression{BreakpointCompression::None};
    
    /// DWord stride of the data, either <format> or <type>
    uint32_t dataDWordStride{0};
};

struct BreakpointHeader {
    /// The execution UID that owns this breakpoint
    uint32_t acquiredExecutionUID{0};

    /// The lock for checksum data
    uint32_t streamingChecksumLock{0};

    /// Expected checksum
    uint32_t streamingChecksum{0};
    
    /// The order of the data
    /// This is typically determined on the device, as it might be driven by memory constraints
    BreakpointDataOrder dataOrder{BreakpointDataOrder::None};

    /// Static ordering layout
    /// Not unioned, to simplify control-flow on breakpoint allocation
    uint32_t staticWidth{0};
    uint32_t staticHeight{0};
    uint32_t staticDepth{0};

    /// Dynamic ordering counter
    uint32_t dynamicCounter{0};

    /// The expected number of streamed dwords
    uint32_t dwordStreamCount{0};

    /// Useful for debugging
#ifndef NDEBUG
    uint32_t debugPayloads[8];
#endif // NDEBUG
};

struct BreakpointPatchData {
    /// The allocation offset
    uint32_t allocationDWordOffset{0};

    /// The total number of offsets for the stream
    uint32_t streamDWordCount{0};
};

/// Number of dwords
static constexpr uint32_t BreakpointDynamicHeaderDWordCount = sizeof(BreakpointDynamicHeader) / sizeof(uint32_t);
static constexpr uint32_t BreakpointStreamingHeaderDWordCount = sizeof(BreakpointHeader) / sizeof(uint32_t);
