#pragma once

// Backend
#include <Backend/IShaderSGUIDHost.h>

// Std
#include <vector>
#include <unordered_map>

// Forward declarations
struct DeviceDispatchTable;
struct SpvSourceMap;

class ShaderSGUIDHost final : public IShaderSGUIDHost {
public:
    explicit ShaderSGUIDHost(DeviceDispatchTable* table);

    bool Install();

    /// Overrides
    ShaderSGUID Bind(const IL::Program &program, const IL::ConstOpaqueInstructionRef& instruction) override;
    ShaderSourceMapping GetMapping(ShaderSGUID sguid) override;
    std::string_view GetSource(ShaderSGUID sguid) override;
    std::string_view GetSource(const ShaderSourceMapping &mapping) override;

private:
    struct ShaderEntry {
        std::unordered_map<ShaderSourceMapping::TSortKey, ShaderSourceMapping> mappings;
    };

    /// Get the source map from a guid
    /// \param shaderGUID the shader guid
    /// \return source map, nullptr if not found
    const SpvSourceMap* GetSourceMap(uint64_t shaderGUID);

private:
    DeviceDispatchTable* table;

    /// All guid to shader entries
    std::unordered_map<uint64_t, ShaderEntry> shaderEntries;

    /// Current allocation counter
    ShaderSGUID counter{0};

    /// Free'd indices to be used immediately
    std::vector<ShaderSGUID> freeIndices;

    /// Reverse sguid lookup
    std::vector<ShaderSourceMapping> sguidLookup;
};
