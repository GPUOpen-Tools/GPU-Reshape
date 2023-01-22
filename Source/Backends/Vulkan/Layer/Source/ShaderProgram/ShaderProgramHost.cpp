#include "Backends/Vulkan/Compiler/ShaderCompilerDebug.h"
#include "Backends/Vulkan/Export/ShaderExportDescriptorAllocator.h"
#include "Backends/Vulkan/ShaderData/ShaderDataHost.h"
#include <Backends/Vulkan/Compiler/SpvModule.h>
#include <Backends/Vulkan/Modules/InbuiltTemplateModuleVulkan.h>
#include <Backends/Vulkan/ShaderProgram/ShaderProgramHost.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>

// Backend
#include <Backend/ShaderProgram/IShaderProgram.h>

// Common
#include <Common/Registry.h>
#include <Common/GlobalUID.h>

ShaderProgramHost::ShaderProgramHost(DeviceDispatchTable *table) : table(table) {

}

bool ShaderProgramHost::Install() {
    templateModule = new(registry->GetAllocators()) SpvModule(allocators, 0u);

    // Attempt to parse template data
    if (!templateModule->ParseModule(
        reinterpret_cast<const uint32_t*>(kSPIRVInbuiltTemplateModuleVulkan),
        static_cast<uint32_t>(sizeof(kSPIRVInbuiltTemplateModuleVulkan) / 4u)
    )) {
        return false;
    }

    // Optional debug
    debug = registry->Get<ShaderCompilerDebug>();

    // OK
    return true;
}

bool ShaderProgramHost::InstallPrograms() {
    // Get the descriptor allocator
    auto shaderExportDescriptorAllocator = registry->Get<ShaderExportDescriptorAllocator>();

    // Get the export host
    auto shaderDataHost = registry->Get<ShaderDataHost>();

    // Get number of resources
    uint32_t resourceCount;
    shaderDataHost->Enumerate(&resourceCount, nullptr, ShaderDataType::All);

    // Fill resources
    shaderData.resize(resourceCount);
    shaderDataHost->Enumerate(&resourceCount, shaderData.data(), ShaderDataType::All);

    // Get number of events
    uint32_t eventCount{0};
    table->dataHost->Enumerate(&eventCount, nullptr, ShaderDataType::Event);

    // Create all programs
    for (ProgramEntry& entry : programs) {
        if (!entry.program) {
            continue;
        }

        // Copy the template module
        entry.module = templateModule->Copy();

        // Get user map
        IL::ShaderDataMap& shaderDataMap = entry.module->GetProgram()->GetShaderDataMap();

        // Add resources
        for (const ShaderDataInfo& info : shaderData) {
            shaderDataMap.Add(info);
        }

        // Finally, inject the host program8
        entry.program->Inject(*entry.module->GetProgram());

        SpvJob spvJob{};
        spvJob.bindingInfo = shaderExportDescriptorAllocator->GetBindingInfo();

        if (!entry.module->Recompile(
            reinterpret_cast<const uint32_t*>(kSPIRVInbuiltTemplateModuleVulkan),
            static_cast<uint32_t>(sizeof(kSPIRVInbuiltTemplateModuleVulkan) / 4u),
            spvJob
        )) {
            return false;
        }

        // Debug
        if (debug) {
            // Allocate path
            std::filesystem::path debugPath = debug->AllocatePath("program");

            // Dump source
            debug->Add(
                debugPath,
                "instrumented",
                entry.module,
                entry.module->GetCode(),
                entry.module->GetSize()
            );

            // Validate the source module
            ENSURE(debug->Validate(
                entry.module->GetCode(),
                entry.module->GetSize() / sizeof(uint32_t)
            ), "Validation failed");
        }

        // Setup shader module
        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = entry.module->GetSize();
        moduleCreateInfo.pCode = entry.module->GetCode();

        // Create shader module
        if (VkResult result = table->next_vkCreateShaderModule(
                table->object,
                &moduleCreateInfo,
                nullptr,
                &entry.shaderModule);
            result != VK_SUCCESS) {
            return false;
        }

        // Get shared layout
        VkDescriptorSetLayout layout = shaderExportDescriptorAllocator->GetLayout();

        // Setup pipeline layout
        VkPipelineLayoutCreateInfo layoutCreateInfo{};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCreateInfo.setLayoutCount = 1;
        layoutCreateInfo.pSetLayouts = &layout;

        // Optional event data
        if (eventCount > 0) {
            VkPushConstantRange range;
            range.offset = 0u;
            range.size = eventCount * sizeof(uint32_t);
            range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

            // Set ranges
            layoutCreateInfo.pushConstantRangeCount = 1;
            layoutCreateInfo.pPushConstantRanges = &range;
        }

        // Create pipeline layout
        if (VkResult result = table->next_vkCreatePipelineLayout(
                table->object,
                &layoutCreateInfo,
                nullptr,
                &entry.layout);
            result != VK_SUCCESS) {
            return false;
        }

        // Setup compute pipeline
        VkComputePipelineCreateInfo computeCreateInfo{};
        computeCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computeCreateInfo.layout = entry.layout;
        computeCreateInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        computeCreateInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        computeCreateInfo.stage.module = entry.shaderModule;
        computeCreateInfo.stage.pName = "main";

        // Create compute pipeline
        if (VkResult result = table->next_vkCreateComputePipelines(
                table->object,
                nullptr,
                1u,
                &computeCreateInfo,
                nullptr,
                &entry.pipeline);
            result != VK_SUCCESS) {
            return false;
        }
    }

    // OK
    return true;
}

ShaderProgramID ShaderProgramHost::Register(const ComRef<IShaderProgram> &program) {
    // Allocate identifier
    ShaderProgramID id;
    if (freeIndices.empty()) {
        id = static_cast<ShaderProgramID>(programs.size());
        programs.emplace_back();
    } else {
        id = freeIndices.back();
        freeIndices.pop_back();
    }

    // Populate entry
    ProgramEntry& entry = programs[id];
    entry.program = program;

    // OK
    return id;
}

void ShaderProgramHost::Deregister(ShaderProgramID program) {
    ProgramEntry& entry = programs[program];

    // Release the module
    destroy(entry.module, registry->GetAllocators());

    // Cleanup entry
    entry = {};

    // Mark as free
    freeIndices.push_back(program);
}
