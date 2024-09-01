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

#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Device.h>
#include <Backends/Vulkan/CommandBuffer.h>
#include <Backends/Vulkan/DescriptorSet.h>
#include <Backends/Vulkan/Queue.h>
#include <Backends/Vulkan/Pipeline.h>
#include <Backends/Vulkan/ShaderModule.h>
#include <Backends/Vulkan/Fence.h>
#include <Backends/Vulkan/RenderPass.h>
#include <Backends/Vulkan/Resource.h>
#include <Backends/Vulkan/Swapchain.h>
#include <Backends/Vulkan/Debug.h>
#include <Backends/Vulkan/Memory.h>

// Std
#include <cstring>

/// Global instantiation
std::mutex                              DeviceDispatchTable::Mutex;
std::map<void *, DeviceDispatchTable *> DeviceDispatchTable::Table;

void DeviceDispatchTable::Populate(PFN_vkGetInstanceProcAddr getInstanceProcAddr, PFN_vkGetDeviceProcAddr getDeviceProcAddr) {
    next_vkGetInstanceProcAddr = getInstanceProcAddr;
    next_vkGetDeviceProcAddr   = getDeviceProcAddr;

    // Populate device
    next_vkDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(getDeviceProcAddr(object, "vkDestroyDevice"));

    // Populate command buffer
    next_vkCreateCommandPool = reinterpret_cast<PFN_vkCreateCommandPool>(getDeviceProcAddr(object, "vkCreateCommandPool"));
    next_vkAllocateCommandBuffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(getDeviceProcAddr(object, "vkAllocateCommandBuffers"));
    next_vkBeginCommandBuffer = reinterpret_cast<PFN_vkBeginCommandBuffer>(getDeviceProcAddr(object, "vkBeginCommandBuffer"));
    next_vkResetCommandBuffer = reinterpret_cast<PFN_vkResetCommandBuffer>(getDeviceProcAddr(object, "vkResetCommandBuffer"));
    next_vkResetCommandPool = reinterpret_cast<PFN_vkResetCommandPool>(getDeviceProcAddr(object, "vkResetCommandPool"));
    next_vkEndCommandBuffer = reinterpret_cast<PFN_vkEndCommandBuffer>(getDeviceProcAddr(object, "vkEndCommandBuffer"));
    next_vkFreeCommandBuffers = reinterpret_cast<PFN_vkFreeCommandBuffers>(getDeviceProcAddr(object, "vkFreeCommandBuffers"));
    next_vkDestroyCommandPool = reinterpret_cast<PFN_vkDestroyCommandPool>(getDeviceProcAddr(object, "vkDestroyCommandPool"));
    next_vkQueueSubmit = reinterpret_cast<PFN_vkQueueSubmit>(getDeviceProcAddr(object, "vkQueueSubmit"));
    next_vkQueueSubmit2 = reinterpret_cast<PFN_vkQueueSubmit2>(getDeviceProcAddr(object, "vkQueueSubmit2"));
    next_vkQueueSubmit2KHR = reinterpret_cast<PFN_vkQueueSubmit2KHR>(getDeviceProcAddr(object, "vkQueueSubmit2KHR"));
    next_vkQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(getDeviceProcAddr(object, "vkQueuePresentKHR"));
    next_vkCreateShaderModule = reinterpret_cast<PFN_vkCreateShaderModule>(getDeviceProcAddr(object, "vkCreateShaderModule"));
    next_vkDestroyShaderModule = reinterpret_cast<PFN_vkDestroyShaderModule>(getDeviceProcAddr(object, "vkDestroyShaderModule"));
    next_vkCreateGraphicsPipelines = reinterpret_cast<PFN_vkCreateGraphicsPipelines>(getDeviceProcAddr(object, "vkCreateGraphicsPipelines"));
    next_vkCreateComputePipelines = reinterpret_cast<PFN_vkCreateComputePipelines>(getDeviceProcAddr(object, "vkCreateComputePipelines"));
    next_vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(getDeviceProcAddr(object, "vkCreateRayTracingPipelinesKHR"));
    next_vkDestroyPipeline = reinterpret_cast<PFN_vkDestroyPipeline>(getDeviceProcAddr(object, "vkDestroyPipeline"));
    next_vkGetFenceStatus = reinterpret_cast<PFN_vkGetFenceStatus>(getDeviceProcAddr(object, "vkGetFenceStatus"));
    next_vkWaitForFences = reinterpret_cast<PFN_vkWaitForFences>(getDeviceProcAddr(object, "vkWaitForFences"));
    next_vkCreateBuffer = reinterpret_cast<PFN_vkCreateBuffer>(getDeviceProcAddr(object, "vkCreateBuffer"));
    next_vkDestroyBuffer = reinterpret_cast<PFN_vkDestroyBuffer>(getDeviceProcAddr(object, "vkDestroyBuffer"));
    next_vkCreateBufferView = reinterpret_cast<PFN_vkCreateBufferView>(getDeviceProcAddr(object, "vkCreateBufferView"));
    next_vkDestroyBufferView = reinterpret_cast<PFN_vkDestroyBufferView>(getDeviceProcAddr(object, "vkDestroyBufferView"));
    next_vkGetBufferMemoryRequirements = reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(getDeviceProcAddr(object, "vkGetBufferMemoryRequirements"));
    next_vkCreateDescriptorPool = reinterpret_cast<PFN_vkCreateDescriptorPool>(getDeviceProcAddr(object, "vkCreateDescriptorPool"));
    next_vkDestroyDescriptorPool = reinterpret_cast<PFN_vkDestroyDescriptorPool>(getDeviceProcAddr(object, "vkDestroyDescriptorPool"));
    next_vkCreateDescriptorSetLayout = reinterpret_cast<PFN_vkCreateDescriptorSetLayout>(getDeviceProcAddr(object, "vkCreateDescriptorSetLayout"));
    next_vkDestroyDescriptorSetLayout = reinterpret_cast<PFN_vkDestroyDescriptorSetLayout>(getDeviceProcAddr(object, "vkDestroyDescriptorSetLayout"));
    next_vkAllocateDescriptorSets = reinterpret_cast<PFN_vkAllocateDescriptorSets>(getDeviceProcAddr(object, "vkAllocateDescriptorSets"));
    next_vkFreeDescriptorSets = reinterpret_cast<PFN_vkFreeDescriptorSets>(getDeviceProcAddr(object, "vkFreeDescriptorSets"));
    next_vkResetDescriptorPool = reinterpret_cast<PFN_vkResetDescriptorPool>(getDeviceProcAddr(object, "vkResetDescriptorPool"));
    next_vkCreatePipelineLayout = reinterpret_cast<PFN_vkCreatePipelineLayout>(getDeviceProcAddr(object, "vkCreatePipelineLayout"));
    next_vkDestroyPipelineLayout = reinterpret_cast<PFN_vkDestroyPipelineLayout>(getDeviceProcAddr(object, "vkDestroyPipelineLayout"));
    next_vkDeviceWaitIdle = reinterpret_cast<PFN_vkDeviceWaitIdle>(getDeviceProcAddr(object, "vkDeviceWaitIdle"));
    next_vkQueueWaitIdle = reinterpret_cast<PFN_vkQueueWaitIdle>(getDeviceProcAddr(object, "vkQueueWaitIdle"));
    next_vkCreateFence = reinterpret_cast<PFN_vkCreateFence>(getDeviceProcAddr(object, "vkCreateFence"));
    next_vkDestroyFence = reinterpret_cast<PFN_vkDestroyFence>(getDeviceProcAddr(object, "vkDestroyFence"));
    next_vkResetFences = reinterpret_cast<PFN_vkResetFences>(getDeviceProcAddr(object, "vkResetFences"));
    next_vkCreateRenderPass = reinterpret_cast<PFN_vkCreateRenderPass>(getDeviceProcAddr(object, "vkCreateRenderPass"));
    next_vkCreateRenderPass2 = reinterpret_cast<PFN_vkCreateRenderPass2>(getDeviceProcAddr(object, "vkCreateRenderPass2"));
    next_vkCreateRenderPass2KHR = reinterpret_cast<PFN_vkCreateRenderPass2>(getDeviceProcAddr(object, "vkCreateRenderPass2KHR"));
    next_vkDestroyRenderPass = reinterpret_cast<PFN_vkDestroyRenderPass>(getDeviceProcAddr(object, "vkDestroyRenderPass"));
    next_vkCreateFrameBuffer = reinterpret_cast<PFN_vkCreateFramebuffer>(getDeviceProcAddr(object, "vkCreateFramebuffer"));
    next_vkDestroyFrameBuffer = reinterpret_cast<PFN_vkDestroyFramebuffer>(getDeviceProcAddr(object, "vkDestroyFramebuffer"));
    next_vkBindBufferMemory = reinterpret_cast<PFN_vkBindBufferMemory>(getDeviceProcAddr(object, "vkBindBufferMemory"));
    next_vkUpdateDescriptorSets = reinterpret_cast<PFN_vkUpdateDescriptorSets>(getDeviceProcAddr(object, "vkUpdateDescriptorSets"));
    next_vkCreateDescriptorUpdateTemplate = reinterpret_cast<PFN_vkCreateDescriptorUpdateTemplate>(getDeviceProcAddr(object, "vkCreateDescriptorUpdateTemplate"));
    next_vkDestroyDescriptorUpdateTemplate = reinterpret_cast<PFN_vkDestroyDescriptorUpdateTemplate>(getDeviceProcAddr(object, "vkDestroyDescriptorUpdateTemplate"));
    next_vkUpdateDescriptorSetWithTemplate = reinterpret_cast<PFN_vkUpdateDescriptorSetWithTemplate>(getDeviceProcAddr(object, "vkUpdateDescriptorSetWithTemplate"));
    next_vkGetDeviceQueue = reinterpret_cast<PFN_vkGetDeviceQueue>(getDeviceProcAddr(object, "vkGetDeviceQueue"));
    next_vkGetDeviceQueue2 = reinterpret_cast<PFN_vkGetDeviceQueue2>(getDeviceProcAddr(object, "vkGetDeviceQueue2"));
    next_vkCreateImage = reinterpret_cast<PFN_vkCreateImage>(getDeviceProcAddr(object, "vkCreateImage"));
    next_vkDestroyImage = reinterpret_cast<PFN_vkDestroyImage>(getDeviceProcAddr(object, "vkDestroyImage"));
    next_vkCreateImageView = reinterpret_cast<PFN_vkCreateImageView>(getDeviceProcAddr(object, "vkCreateImageView"));
    next_vkDestroyImageView = reinterpret_cast<PFN_vkDestroyImageView>(getDeviceProcAddr(object, "vkDestroyImageView"));
    next_vkCreateSampler = reinterpret_cast<PFN_vkCreateSampler>(getDeviceProcAddr(object, "vkCreateSampler"));
    next_vkDestroySampler = reinterpret_cast<PFN_vkDestroySampler>(getDeviceProcAddr(object, "vkDestroySampler"));
    next_vkAllocateMemory = reinterpret_cast<PFN_vkAllocateMemory>(getDeviceProcAddr(object, "vkAllocateMemory"));
    next_vkFreeMemory = reinterpret_cast<PFN_vkFreeMemory>(getDeviceProcAddr(object, "vkFreeMemory"));
    next_vkBindBufferMemory2KHR = reinterpret_cast<PFN_vkBindBufferMemory2KHR>(getDeviceProcAddr(object, "vkBindBufferMemory2KHR"));
    next_vkBindImageMemory = reinterpret_cast<PFN_vkBindImageMemory>(getDeviceProcAddr(object, "vkBindImageMemory"));
    next_vkBindImageMemory2KHR = reinterpret_cast<PFN_vkBindImageMemory2KHR>(getDeviceProcAddr(object, "vkBindImageMemory2KHR"));
    next_vkFlushMappedMemoryRanges = reinterpret_cast<PFN_vkFlushMappedMemoryRanges>(getDeviceProcAddr(object, "vkFlushMappedMemoryRanges"));
    next_vkGetBufferMemoryRequirements2KHR = reinterpret_cast<PFN_vkGetBufferMemoryRequirements2KHR>(getDeviceProcAddr(object, "vkGetBufferMemoryRequirements2KHR"));
    next_vkGetImageMemoryRequirements = reinterpret_cast<PFN_vkGetImageMemoryRequirements>(getDeviceProcAddr(object, "vkGetImageMemoryRequirements"));
    next_vkGetImageMemoryRequirements2KHR = reinterpret_cast<PFN_vkGetImageMemoryRequirements2KHR>(getDeviceProcAddr(object, "vkGetImageMemoryRequirements2KHR"));
    next_vkInvalidateMappedMemoryRanges = reinterpret_cast<PFN_vkInvalidateMappedMemoryRanges>(getDeviceProcAddr(object, "vkInvalidateMappedMemoryRanges"));
    next_vkMapMemory = reinterpret_cast<PFN_vkMapMemory>(getDeviceProcAddr(object, "vkMapMemory"));
    next_vkUnmapMemory = reinterpret_cast<PFN_vkUnmapMemory>(getDeviceProcAddr(object, "vkUnmapMemory"));
    next_vkCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(getDeviceProcAddr(object, "vkCreateSwapchainKHR"));
    next_vkDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(getDeviceProcAddr(object, "vkDestroySwapchainKHR"));
    next_vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(getDeviceProcAddr(object, "vkGetSwapchainImagesKHR"));
    next_vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(getDeviceProcAddr(object, "vkSetDebugUtilsObjectNameEXT"));
    next_vkDebugMarkerSetObjectNameEXT = reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(getDeviceProcAddr(object, "vkDebugMarkerSetObjectNameEXT"));
    next_vkQueueBindSparse = reinterpret_cast<PFN_vkQueueBindSparse>(getDeviceProcAddr(object, "vkQueueBindSparse"));
    next_vkCreateSemaphore = reinterpret_cast<PFN_vkCreateSemaphore>(getDeviceProcAddr(object, "vkCreateSemaphore"));
    next_vkDestroySemaphore = reinterpret_cast<PFN_vkDestroySemaphore>(getDeviceProcAddr(object, "vkDestroySemaphore"));

    // Populate all generated commands
    commandBufferDispatchTable.Populate(object, getDeviceProcAddr);
}

PFN_vkVoidFunction DeviceDispatchTable::GetHookAddress(DeviceDispatchTable* table, const char *name) {
    if (!std::strcmp(name, "vkCreateDevice"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateDevice);

    if (!std::strcmp(name, "vkDestroyDevice"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyDevice);

    if (!std::strcmp(name, "vkCreateShaderModule"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateShaderModule);

    if (!std::strcmp(name, "vkDestroyShaderModule"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyShaderModule);

    if (!std::strcmp(name, "vkCreateGraphicsPipelines"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateGraphicsPipelines);

    if (!std::strcmp(name, "vkCreateComputePipelines"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateComputePipelines);

    if (!std::strcmp(name, "vkCreateRayTracingPipelinesKHR"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateRayTracingPipelinesKHR);

    if (!std::strcmp(name, "vkDestroyPipeline"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyPipeline);

    if (!std::strcmp(name, "vkCreateCommandPool"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateCommandPool);

    if (!std::strcmp(name, "vkAllocateCommandBuffers"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkAllocateCommandBuffers);

    if (!std::strcmp(name, "vkBeginCommandBuffer"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkBeginCommandBuffer);

    if (!std::strcmp(name, "vkResetCommandBuffer"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkResetCommandBuffer);

    if (!std::strcmp(name, "vkResetCommandPool"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkResetCommandPool);

    if (!std::strcmp(name, "vkEndCommandBuffer"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkEndCommandBuffer);

    if (!std::strcmp(name, "vkFreeCommandBuffers"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkFreeCommandBuffers);

    if (!std::strcmp(name, "vkDestroyCommandPool"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyCommandPool);

    if (!std::strcmp(name, "vkQueueSubmit"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkQueueSubmit);

    if (!std::strcmp(name, "vkQueueSubmit2"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkQueueSubmit2);

    if (!std::strcmp(name, "vkQueueSubmit2KHR"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkQueueSubmit2);

    if (!std::strcmp(name, "vkQueuePresentKHR"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkQueuePresentKHR);

    if (!std::strcmp(name, "vkCreatePipelineLayout"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreatePipelineLayout);

    if (!std::strcmp(name, "vkCreateDescriptorSetLayout"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateDescriptorSetLayout);

    if (!std::strcmp(name, "vkDestroyDescriptorSetLayout"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyDescriptorSetLayout);

    if (!std::strcmp(name, "vkDestroyPipelineLayout"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyPipelineLayout);

    if (!std::strcmp(name, "vkDeviceWaitIdle"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDeviceWaitIdle);

    if (!std::strcmp(name, "vkGetDeviceQueue"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkGetDeviceQueue);

    if (!std::strcmp(name, "vkGetDeviceQueue2"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkGetDeviceQueue2);

    if (!std::strcmp(name, "vkQueueWaitIdle"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkQueueWaitIdle);

    if (!std::strcmp(name, "vkCreateFence"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateFence);

    if (!std::strcmp(name, "vkDestroyFence"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyFence);

    if (!std::strcmp(name, "vkGetFenceStatus"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkGetFenceStatus);

    if (!std::strcmp(name, "vkWaitForFences"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkWaitForFences);

    if (!std::strcmp(name, "vkResetFences"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkResetFences);

    if (!std::strcmp(name, "vkCreateRenderPass"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateRenderPass);

    if (!std::strcmp(name, "vkCreateRenderPass2"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateRenderPass2);

    if (!std::strcmp(name, "vkCreateRenderPass2KHR"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateRenderPass2KHR);

    if (!std::strcmp(name, "vkDestroyRenderPass"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyRenderPass);

    if (!std::strcmp(name, "vkCreateFramebuffer"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateFramebuffer);

    if (!std::strcmp(name, "vkDestroyFramebuffer"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyFramebuffer);

    if (!std::strcmp(name, "vkCreateDescriptorPool"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateDescriptorPool);

    if (!std::strcmp(name, "vkDestroyDescriptorPool"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyDescriptorPool);

    if (!std::strcmp(name, "vkResetDescriptorPool"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkResetDescriptorPool);

    if (!std::strcmp(name, "vkAllocateDescriptorSets"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkAllocateDescriptorSets);

    if (!std::strcmp(name, "vkFreeDescriptorSets"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkFreeDescriptorSets);

    if (!std::strcmp(name, "vkUpdateDescriptorSets"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkUpdateDescriptorSets);

    if (!std::strcmp(name, "vkCreateDescriptorUpdateTemplate"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateDescriptorUpdateTemplate);

    if (!std::strcmp(name, "vkDestroyDescriptorUpdateTemplate"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyDescriptorUpdateTemplate);

    if (!std::strcmp(name, "vkUpdateDescriptorSetWithTemplate"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkUpdateDescriptorSetWithTemplate);

    if (!std::strcmp(name, "vkCreateBuffer"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateBuffer);

    if (!std::strcmp(name, "vkCreateSwapchainKHR"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateSwapchainKHR);

    if (!std::strcmp(name, "vkDestroySwapchainKHR"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroySwapchainKHR);

    if (!std::strcmp(name, "vkSetDebugUtilsObjectNameEXT"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkSetDebugUtilsObjectNameEXT);

    if (!std::strcmp(name, "vkDebugMarkerSetObjectNameEXT"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDebugMarkerSetObjectNameEXT);

    if (!std::strcmp(name, "vkCreateBufferView"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateBufferView);

    if (!std::strcmp(name, "vkCreateImage"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateImage);

    if (!std::strcmp(name, "vkCreateImageView"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateImageView);

    if (!std::strcmp(name, "vkCreateSampler"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateSampler);

    if (!std::strcmp(name, "vkDestroyBuffer"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyBuffer);

    if (!std::strcmp(name, "vkDestroyBufferView"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyBufferView);

    if (!std::strcmp(name, "vkDestroyImage"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyImage);

    if (!std::strcmp(name, "vkDestroyImageView"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyImageView);

    if (!std::strcmp(name, "vkDestroySampler"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroySampler);

    if (!std::strcmp(name, "vkAllocateMemory"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkAllocateMemory);

    if (!std::strcmp(name, "vkFreeMemory"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkFreeMemory);

    if (!std::strcmp(name, "vkMapMemory"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkMapMemory);

    if (!std::strcmp(name, "vkUnmapMemory"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkUnmapMemory);

    if (!std::strcmp(name, "vkBindBufferMemory"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkBindBufferMemory);

    if (!std::strcmp(name, "vkBindImageMemory"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkBindImageMemory);

    if (!std::strcmp(name, "vkBindBufferMemory2KHR"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkBindBufferMemory2KHR);

    if (!std::strcmp(name, "vkBindImageMemory2KHR"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkBindImageMemory2KHR);

    // Check command hooks
    if (PFN_vkVoidFunction hook = CommandBufferDispatchTable::GetHookAddress(table ? &table->commandBufferDispatchTable : nullptr, name)) {
        return hook;
    }

    // No hook
    return nullptr;
}
