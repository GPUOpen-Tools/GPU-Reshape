#pragma once

// Common
#include <Common/IComponent.h>

// Std
#include <filesystem>

class SpvModule;
struct InstanceDispatchTable;
struct VkShaderModuleCreateInfo;

class ShaderCompilerDebug : public TComponent<ShaderCompilerDebug> {
public:
    COMPONENT(ShaderCompilerDebug);

    ShaderCompilerDebug(InstanceDispatchTable* table);

    /// Install this component
    /// \return
    bool Install();

    /// Validate a module
    /// \param module
    /// \return
    bool Validate(SpvModule* module);

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

    /// Base path for all debugging data
    std::filesystem::path path;
};
