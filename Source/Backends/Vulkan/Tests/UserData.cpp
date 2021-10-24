#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "Loader.h"

TEST_CASE_METHOD(Loader, "User Data", "[Vulkan]") {
    REQUIRE(AddInstanceLayer("GBV_user_data_layer"));

    // Create the instance
    CreateInstance();

    // Try to add VK_EXT_private_data
    const bool SupportsPrivateData = AddDeviceExtension(VK_EXT_PRIVATE_DATA_EXTENSION_NAME);

    // Create the device with the given extensions
    CreateDevice();

    VkCommandPoolCreateInfo info{};
    info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.queueFamilyIndex = GetPrimaryQueueFamily();

    // Attempt to create the pool
    VkCommandPool pool;
    REQUIRE(vkCreateCommandPool(GetDevice(), &info, nullptr, &pool) == VK_SUCCESS);

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandPool        = pool;
    allocateInfo.commandBufferCount = 1;

    // Attempt to allocate the command buffer
    VkCommandBuffer commandBuffer;
    REQUIRE(vkAllocateCommandBuffers(GetDevice(), &allocateInfo, &commandBuffer) == VK_SUCCESS);

    VkMemoryBarrier barrier{};
    barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_HOST_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_WRITE_BIT;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkPipelineLayout layout;
    REQUIRE(vkCreatePipelineLayout(GetDevice(), &layoutInfo, nullptr, &layout) == VK_SUCCESS);

    // Empty compute kernel, SPIRV
    static constexpr uint32_t kCode[] =
    {
        0x07230203,0x00010000,0x000d000a,0x0000000a,
        0x00000000,0x00020011,0x00000001,0x0006000b,
        0x00000001,0x4c534c47,0x6474732e,0x3035342e,
        0x00000000,0x0003000e,0x00000000,0x00000001,
        0x0005000f,0x00000005,0x00000004,0x6e69616d,
        0x00000000,0x00060010,0x00000004,0x00000011,
        0x00000001,0x00000001,0x00000001,0x00030003,
        0x00000002,0x000001b8,0x000a0004,0x475f4c47,
        0x4c474f4f,0x70635f45,0x74735f70,0x5f656c79,
        0x656e696c,0x7269645f,0x69746365,0x00006576,
        0x00080004,0x475f4c47,0x4c474f4f,0x6e695f45,
        0x64756c63,0x69645f65,0x74636572,0x00657669,
        0x00040005,0x00000004,0x6e69616d,0x00000000,
        0x00040047,0x00000009,0x0000000b,0x00000019,
        0x00020013,0x00000002,0x00030021,0x00000003,
        0x00000002,0x00040015,0x00000006,0x00000020,
        0x00000000,0x00040017,0x00000007,0x00000006,
        0x00000003,0x0004002b,0x00000006,0x00000008,
        0x00000001,0x0006002c,0x00000007,0x00000009,
        0x00000008,0x00000008,0x00000008,0x00050036,
        0x00000002,0x00000004,0x00000000,0x00000003,
        0x000200f8,0x00000005,0x000100fd,0x00010038
    };

    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = sizeof(kCode);
    moduleCreateInfo.pCode = kCode;

    VkShaderModule module;
    REQUIRE(vkCreateShaderModule(GetDevice(), &moduleCreateInfo, nullptr, &module) == VK_SUCCESS);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.pName = "main";
    stageInfo.module = module;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = layout;
    pipelineInfo.stage  = stageInfo;

    VkPipeline pipeline;
    REQUIRE(vkCreateComputePipelines(GetDevice(), nullptr, 1, &pipelineInfo, nullptr, &pipeline) == VK_SUCCESS);

    SECTION("Dispatch Table")
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        REQUIRE(vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

        BENCHMARK("Lookup Table") {
            vkCmdDispatch(commandBuffer, 0, 1, 1);
        };

        BENCHMARK("Wrapped Object") {
            vkCmdDispatch(commandBuffer, 1, 1, 1);
        };

        if (SupportsPrivateData) {
            BENCHMARK("Private Data") {
                  vkCmdDispatch(commandBuffer, 2, 1, 1);
              };
        }

        REQUIRE(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS);
    }

    SECTION("Features")
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        REQUIRE(vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

        BENCHMARK("Baseline") {
            vkCmdDispatchIndirect(commandBuffer, nullptr, 0);
        };

        BENCHMARK("Std Vector") {
            vkCmdDispatchIndirect(commandBuffer, nullptr, 1);
        };

        BENCHMARK("Flat Array") {
            vkCmdDispatchIndirect(commandBuffer, nullptr, 2);
        };

        BENCHMARK("Std Vector, Zero feature set") {
            vkCmdDispatchIndirect(commandBuffer, nullptr, 3);
        };

        BENCHMARK("Std Vector, Bit loop") {
            vkCmdDispatchIndirect(commandBuffer, nullptr, 4);
        };

        BENCHMARK("Std Vector, Many features, null check") {
            vkCmdDispatchIndirect(commandBuffer, nullptr, 5);
        };

        BENCHMARK("Std Vector, Many features, bit loop") {
            vkCmdDispatchIndirect(commandBuffer, nullptr, 6);
        };

        BENCHMARK("Std Vector, Many features, few enabled, bit loop") {
            vkCmdDispatchIndirect(commandBuffer, nullptr, 7);
        };

        REQUIRE(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS);
    }
}

