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

#include <Backends/Vulkan/Programs/RaytracingPrograms.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Modules/ShaderRecordPatchingVulkan.h>

static bool CreateSBTPatchProgram(const Allocators& allocators, DeviceDispatchTable* table, RaytracingPrograms& program) {
    // General descriptor layout
    VkDescriptorSetLayoutBinding descriptorBindings[] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
        },
        {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
        },
        {
            .binding = 3,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
        }
    };

    // Set layout info
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    descriptorSetLayoutInfo.bindingCount = 4;
    descriptorSetLayoutInfo.pBindings = descriptorBindings;
    table->next_vkCreateDescriptorSetLayout(table->object, &descriptorSetLayoutInfo, nullptr, &program.sbtPatchSetLayout);

    // Pipeline layout info
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &program.sbtPatchSetLayout;
    table->next_vkCreatePipelineLayout(table->object, &pipelineLayoutInfo, nullptr, &program.sbtPatchPipelineLayout);

    // Module info
    VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    moduleInfo.codeSize = sizeof(kShaderRecordPatchingVulkan);
    moduleInfo.pCode = reinterpret_cast<const uint32_t *>(kShaderRecordPatchingVulkan);

    // Try to create module
    VkShaderModule module;
    table->next_vkCreateShaderModule(table->object, &moduleInfo, nullptr, &module);

    // Try to create pipeline
    VkComputePipelineCreateInfo computePipelineInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    computePipelineInfo.layout = program.sbtPatchPipelineLayout;
    computePipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computePipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computePipelineInfo.stage.module = module;
    computePipelineInfo.stage.pName = "main";
    table->next_vkCreateComputePipelines(table->object, nullptr, 1, &computePipelineInfo, nullptr, &program.sbtPatchPipeline);

    // OK
    return true;
}

bool CreateRaytracingPrograms(const Allocators& allocators, DeviceDispatchTable* table, RaytracingPrograms& program) {
    // Create the patching programs
    if (!CreateSBTPatchProgram(allocators, table, program)) {
        return false;
    }

    // OK
    return true;
}

RaytracingPrograms::~RaytracingPrograms() {
    
}
