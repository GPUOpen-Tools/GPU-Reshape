#pragma once

// Common
#include <Common/IComponent.h>

// Std
#include <filesystem>
#include <mutex>

// Forward declarations
class DXModule;
struct DeviceState;

class ShaderCompilerDebug : public TComponent<ShaderCompilerDebug> {
public:
    COMPONENT(ShaderCompilerDebug);

    ShaderCompilerDebug();

    /// Allocate
    /// \param module module to be allocated for
    /// \return
    std::filesystem::path AllocatePath(DXModule *module);

    /// Add a module
    /// \param basePath allocated base path
    /// \param category category of the shader module
    /// \param module module to be serialized
    void Add(const std::filesystem::path& basePath, const std::string_view& category, DXModule *module);

private:
    /// Shared lock for printing
    std::mutex sharedLock;

    /// Base path for all debugging data
    std::filesystem::path path;
};
