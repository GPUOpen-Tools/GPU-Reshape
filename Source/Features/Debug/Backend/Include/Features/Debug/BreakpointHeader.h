#pragma once

// Backend
#include <Backend/IL/Format.h>
#include <Backend/IL/Type.h>
#include <Backend/IL/Execution/ExecutionInfo.h>

// Common
#include <Common/Enum.h>

// Std
#include <cstdint>


/**
 * Source Mirrors
 * - Source/Features/Debug/Frontend/UIX/Models/BreakpointHeader.cs
 */
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

enum class BreakpointCaptureMode {
    /// Capture the first event only
    FirstEvent,

    /// Capture all events
    AllEvents
};

BIT_SET(BreakpointFlag);

enum class BreakpointDataOrder {
    None,

    /// Statically known ordering
    /// Each data piece doesn't need to encode its location, it's known implicitly
    Static,

    /// Dynamic ordering
    Dynamic,

    /// Loose ordering
    Loose,
};

struct BreakpointDynamicHeader {
    /// Thread index that exported the data
    /// Given that we know the dimensionality of the data, we can infer it from the linear index
    uint32_t thread;
};

struct BreakpointLooseHeader {
    /// Executing data
    ExecutionInfo info;

    /// Thread indices
    uint32_t threadX;
    uint32_t threadY;
    uint32_t threadZ;
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

    /// Starting offset of the payload dwords
    uint32_t payloadDWordOffset{0};

    /// Total number of payload dwords
    uint32_t payloadDWordCount{0};

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
    uint32_t paddingPayload[5];
};

struct BreakpointPatchData {
    /// The allocation offset
    uint32_t allocationDWordOffset{0};

    /// The total number of offsets for the stream
    uint32_t streamDWordCount{0};
};

/// Number of dwords
static constexpr uint32_t BreakpointHeaderDWordCount          = sizeof(BreakpointHeader) / sizeof(uint32_t);
static constexpr uint32_t BreakpointDynamicHeaderDWordCount   = sizeof(BreakpointDynamicHeader) / sizeof(uint32_t);
static constexpr uint32_t BreakpointLooseHeaderDWordCount     = sizeof(BreakpointLooseHeader) / sizeof(uint32_t);
static constexpr uint32_t BreakpointStreamingHeaderDWordCount = sizeof(BreakpointHeader) / sizeof(uint32_t);

/// Validation
static_assert(sizeof(BreakpointHeader) == 64, "Unexpected size");
