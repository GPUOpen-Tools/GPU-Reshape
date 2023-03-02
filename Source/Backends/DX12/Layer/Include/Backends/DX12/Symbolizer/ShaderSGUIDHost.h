#pragma once

// Backend
#include <Backend/IShaderSGUIDHost.h>

// Common
#include <Common/Allocator/Vector.h>

// Std
#include <vector>
#include <unordered_map>
#include <mutex>

// Forward declarations
struct DeviceState;
class IDXDebugModule;
class IBridge;

class ShaderSGUIDHost final : public IShaderSGUIDHost {
public:
    explicit ShaderSGUIDHost(DeviceState* table);

    /// Install this host
    /// \return success state
    bool Install();

    /// Commit all messages
    /// \param bridge
    void Commit(IBridge* bridge);

    /// Overrides
    ShaderSGUID Bind(const IL::Program &program, const IL::ConstOpaqueInstructionRef& instruction) override;
    ShaderSourceMapping GetMapping(ShaderSGUID sguid) override;
    std::string_view GetSource(ShaderSGUID sguid) override;
    std::string_view GetSource(const ShaderSourceMapping &mapping) override;

private:
    /// Cached entry maps
    struct ShaderEntry {
        std::unordered_map<ShaderSourceMapping::TSortKey, ShaderSourceMapping> mappings;
    };

private:
    /// Parent device
    DeviceState* device;

    /// Shared lock
    std::mutex mutex;

    /// All guid to shader entries
    std::unordered_map<uint64_t, ShaderEntry> shaderEntries;

    /// Current allocation counter
    ShaderSGUID counter{0};

    /// Free'd indices to be used immediately
    Vector<ShaderSGUID> freeIndices;

    /// Reverse sguid lookup
    Vector<ShaderSourceMapping> sguidLookup;

    /// All pending bridge submissions
    Vector<ShaderSGUID> pendingSubmissions;
};
