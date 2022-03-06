#pragma once

// Common
#include <Common/IComponent.h>

// Std
#include <filesystem>
#include <mutex>

class SpvModule;
struct InstanceDispatchTable;
struct VkShaderModuleCreateInfo;

namespace spvtools {
    class SpirvTools;
}

class ShaderCompilerDebug : public TComponent<ShaderCompilerDebug> {
public:
    COMPONENT(ShaderCompilerDebug);

    ShaderCompilerDebug(InstanceDispatchTable* table);

    ~ShaderCompilerDebug();

    /// Install this component
    /// \return
    bool Install();

    /// Validate a module
    /// \param module
    /// \return
    bool Validate(const uint32_t* spirvCode, uint32_t spirvSize);

    /// Allocate
    /// \param module module to be allocated for
    /// \return
    std::filesystem::path AllocatePath(SpvModule *module);

    /// Add a module
    /// \param basePath allocated base path
    /// \param category category of the shader module
    /// \param module module to be serialized
    /// \param spirvCode produced spirv code
    /// \param spirvSize produced spirv size
    void Add(const std::filesystem::path& basePath, const std::string_view& category, SpvModule *module, const void* spirvCode, uint32_t spirvSize);

private:
    InstanceDispatchTable* table;

    /// Shared lock for printing
    std::mutex sharedLock;

    /// Tools handle
    spvtools::SpirvTools* tools{nullptr};

    /// Base path for all debugging data
    std::filesystem::path path;
};
