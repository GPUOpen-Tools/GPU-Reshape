// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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

        // Setup job description
        SpvJob spvJob{};
        spvJob.bindingInfo = shaderExportDescriptorAllocator->GetBindingInfo();
        spvJob.requiresUserDescriptorMapping = false;

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

        VkPushConstantRange range;
        range.offset = 0u;
        range.size = eventCount * sizeof(uint32_t);
        range.stageFlags = VK_SHADER_STAGE_ALL;

#if PRMT_METHOD == PRMT_METHOD_UB_PC
        // Take single dword for PRMT sub-segment offset
        range.size += sizeof(uint32_t);
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC

        // Optional data
        if (range.size > 0) {
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
