#pragma once

// Layer
#include <Backends/Vulkan/Layer.h>

// Backend
#include <Backend/ShaderProgram/IShaderProgramHost.h>
#include <Backend/ShaderData/ShaderDataInfo.h>

// Std
#include <vector>

// Forward declarations
class SpvModule;
class ShaderDataHost;
class ShaderCompilerDebug;
struct DeviceDispatchTable;

class ShaderProgramHost final : public IShaderProgramHost {
public:
    ShaderProgramHost(DeviceDispatchTable* table);

    /// Install this host
    /// \return success state
    bool Install();

    /// Install this host
    /// \return success state
    bool InstallPrograms();

    /// IShaderProgramHost
    ShaderProgramID Register(const ComRef<IShaderProgram> &program) override;
    void Deregister(ShaderProgramID program) override;

    /// Get the pipeline layout of a program
    VkPipelineLayout GetPipelineLayout(ShaderProgramID id) {
        return programs[id].layout;
    }

    /// Get the pipeline of a program
    VkPipeline GetPipeline(ShaderProgramID id) {
        return programs[id].pipeline;
    }

private:
    struct ProgramEntry {
        /// Templated module
        SpvModule* module{nullptr};

        /// Program shader module
        VkShaderModule shaderModule{VK_NULL_HANDLE};

        /// Program layout
        VkPipelineLayout layout{VK_NULL_HANDLE};

        /// Program pipeline
        VkPipeline pipeline{VK_NULL_HANDLE};

        /// Source program
        ComRef<IShaderProgram> program;
    };

    /// All programs, may contain empty slots
    std::vector<ProgramEntry> programs;

    /// Free program indices
    std::vector<ShaderProgramID> freeIndices;

private:
    /// All exposed shader data
    std::vector<ShaderDataInfo> shaderData;

private:
    /// Base module being templated against
    SpvModule* templateModule{nullptr};

    /// Optional debug handle
    ComRef<ShaderCompilerDebug> debug;

private:
    DeviceDispatchTable* table{nullptr};
};
