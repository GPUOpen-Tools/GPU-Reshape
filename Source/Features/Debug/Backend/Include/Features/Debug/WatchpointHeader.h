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
 * - Source/Features/Debug/Frontend/UIX/Models/WatchpointHeader.cs
 */
enum class WatchpointFlag {
    None = 0,

    /// Allow floating point image compression to 8-8-8-8 texels
    AllowImageFPUNorm8888Compression = BIT(0),

    /// Enables early depth stencil
    EarlyDepthStencil = BIT(1),
};

enum class WatchpointCompression {
    None,

    /// Using fp 8-8-8-8 compression
    FPUnorm8888
};

enum class WatchpointCaptureMode {
    /// Capture the first event only
    /// Synchronization: Per-event, on acq. it's guaranteed
    FirstEvent,

    /// Capture the first viewport only
    /// Synchronization: Per-viewport, on acq. it's guaranteed
    FirstViewport,

    /// Capture all events
    /// Synchronization: Free form, on acq. up to range N.
    AllEvents
};

BIT_SET(WatchpointFlag);

enum class WatchpointDataOrder {
    None,

    /// Statically known ordering
    /// Each data piece doesn't need to encode its location, it's known implicitly
    Static,

    /// Dynamic ordering
    Dynamic,

    /// Loose ordering
    Loose,
};

struct WatchpointDynamicHeader {
    /// Thread index that exported the data
    /// Given that we know the dimensionality of the data, we can infer it from the linear index
    uint32_t thread;
};

struct WatchpointLooseHeader {
    /// Executing data
    ExecutionInfo info;

    /// Thread indices
    uint32_t threadX;
    uint32_t threadY;
    uint32_t threadZ;
};

struct WatchpointDataHostLayout {
    /// Texel format, None if structural
    Backend::IL::Format format{Backend::IL::Format::None};

    /// If format is None, represents the structural type
    IL::ID typeId{IL::InvalidID};

    /// Packed tiny type data
    std::vector<uint8_t> tinyType;

    /// Compression type
    WatchpointCompression compression{WatchpointCompression::None};
    
    /// DWord stride of the data, either <format> or <type>
    uint32_t dataDWordStride{0};
};

struct WatchpointHeader {
    /// The execution UID that owns this watchpoint
    uint32_t acquiredExecutionUID{0};

    /// Starting offset of the payload dwords
    uint32_t payloadDWordOffset{0};

    /// Total number of payload dwords
    uint32_t payloadDWordCount{0};

    /// The data payload dword stride
    uint32_t payloadDataDWordStride{0};

    /// The instrumentation version acquired
    /// Only used for loose data ordering
    uint32_t shaderInstrumentationHash32{0};
    
    /// The order of the data
    /// This is typically determined on the device, as it might be driven by memory constraints
    WatchpointDataOrder dataOrder{WatchpointDataOrder::None};

    /// Static ordering layout
    /// Not unioned, to simplify control-flow on watchpoint allocation
    uint32_t staticWidth{0};
    uint32_t staticHeight{0};
    uint32_t staticDepth{0};

    /// Dynamic ordering counter
    uint32_t dynamicCounter{0};

    /// The expected number of streamed dwords
    uint32_t dwordStreamCount{0};
    
    /// Indirect dispatch parameters
    uint32_t copyDispatchParams[3];
    uint32_t copyDispatchLock{0};
    
    /// Alignment pad
    uint32_t pad{0};
    
    /// Predication arguments
    uint32_t predicationLo{0};
    uint32_t predicationHi{0};
    
    /// Useful for debugging
    uint32_t paddingPayload[2];
};

struct WatchpointLooseAcquisitionData {
    /// The allocation offset
    uint32_t allocationDWordOffset{0};

    /// The watchpoint uid
    uint32_t watchpointUid{0};
};

struct WatchpointResetHeaderData {
    /// The allocation offset
    uint32_t allocationDWordOffset{0};
    
    /// Structural pad
    uint32_t pad{0};
};

struct WatchpointCopyData {
    /// The allocation offset
    uint32_t allocationDWordOffset{0};
    
    /// Structural pad
    uint32_t pad{0};
};

struct WatchpointLockDynamicData {
    /// The allocation offset
    uint32_t allocationDWordOffset{0};
    
    /// Structural pad
    uint32_t pad{0};
};

static constexpr uint32_t kWatchpointInstrumentationHashLocked = UINT32_MAX;

/// Number of dwords
static constexpr uint32_t WatchpointHeaderDWordCount          = sizeof(WatchpointHeader) / sizeof(uint32_t);
static constexpr uint32_t WatchpointDynamicHeaderDWordCount   = sizeof(WatchpointDynamicHeader) / sizeof(uint32_t);
static constexpr uint32_t WatchpointLooseHeaderDWordCount     = sizeof(WatchpointLooseHeader) / sizeof(uint32_t);
static constexpr uint32_t WatchpointStreamingHeaderDWordCount = sizeof(WatchpointHeader) / sizeof(uint32_t);

/// Validation
static_assert(offsetof(WatchpointHeader, predicationLo) % 8 == 0, "Unexpected predication alignment");
static_assert(sizeof(WatchpointHeader)                  % 8 == 0, "Unexpected header alignment");
static_assert(sizeof(WatchpointHeader)                      == 80, "Unexpected size");
