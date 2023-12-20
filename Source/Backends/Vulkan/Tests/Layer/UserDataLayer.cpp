// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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


#include <vulkan/vk_layer.h>

// Std
#include <cstring>
#include <mutex>
#include <map>
#include <vector>
#include <intrin.h>

/**
 * Simple layer for testing lookup tables, object wrapping and private data
 *
 * TODO: Cleanup allocations and whatnot
 */

/// Name of the layer
#define LAYER_NAME "VK_GPUOpen_Test_UserDataLayer"

/// Linkage information
#if defined(_MSC_VER)
#	define EXPORT_C __declspec(dllexport)
#else
#	define EXPORT_C __attribute__((visibility("default")))
#endif

/// Get the internally stored table
template<typename T>
inline void *&GetInternalTable(T inst) {
    return *(void **) inst;
}

/// Simple instance dispatch table
struct InstanceDispatchTable {
    static void Add(void *key, InstanceDispatchTable *table) {
        std::lock_guard<std::mutex> lock(Mutex);
        Table[key] = table;
    }

    static InstanceDispatchTable *Get(void *key) {
        std::lock_guard<std::mutex> lock(Mutex);
        return Table[key];
    }

    PFN_vkGetInstanceProcAddr                getInstanceProcAddr;
    PFN_vkDestroyInstance                    destroyInstance;
    PFN_vkEnumerateDeviceExtensionProperties enumerateDeviceExtensionProperties;

private:
    static std::mutex Mutex;
    static std::map<void *, InstanceDispatchTable *> Table;
};

class IFeature
{
public:
    virtual void Foo() = 0;
    virtual ~IFeature() = default;

    // Function pointer test
    void(*fooFPtr)(IFeature* frame){nullptr};
};

class FeatureA final : public IFeature
{
public:
    FeatureA()
    {
        fooFPtr = [](IFeature* frame)
        {
            static_cast<FeatureA*>(frame)->Foo();
        };
    }

    void Foo() final
    {

    }
};

/// Simple device dispatch table
struct DeviceDispatchTable {
    static void Add(void *key, DeviceDispatchTable *table) {
        std::lock_guard<std::mutex> lock(Mutex);
        Table[key] = table;
    }

    static DeviceDispatchTable *Get(void *key) {
        std::lock_guard<std::mutex> lock(Mutex);
        return Table[key];
    }


    /// Proc addresses
    PFN_vkCmdBindPipeline           cmdBindPipeline;
    PFN_vkCreatePrivateDataSlotEXT  createPrivateDataSlotEXT;
    PFN_vkDestroyPrivateDataSlotEXT destroyPrivateDataSlotEXT;
    PFN_vkSetPrivateDataEXT         setPrivateDataEXT;
    PFN_vkGetPrivateDataEXT         getPrivateDataEXT;
    PFN_vkCmdDispatch               cmdDispatch;
    PFN_vkCmdDispatchIndirect       cmdDispatchIndirect;
    PFN_vkAllocateCommandBuffers    allocateCommandBuffers;
    PFN_vkFreeCommandBuffers        freeCommandBuffers;
    PFN_vkBeginCommandBuffer        beginCommandBuffer;
    PFN_vkEndCommandBuffer          endCommandBuffer;
    PFN_vkGetDeviceProcAddr         getDeviceProcAddr;
    PFN_vkDestroyDevice             destroyDevice;

    /// Data
    VkDevice             device;
    VkPrivateDataSlotEXT slot;

    /// Feature
    IFeature* feature{};

    /// Feature testing, vector
    std::vector<IFeature*> dispatchIndirectFeatures;

    /// Feature testing, inline
    IFeature* dispatchIndirectFeaturesFlat[1];
    uint32_t dispatchIndirectFeaturesFlatCount;

    /// Feature testing, bitset
    uint64_t dispatchIndirectFeatureSetZero = 0;

    /// Feature testing, bitset
    uint64_t dispatchIndirectFeatureSetBits = 0;
    std::vector<IFeature*> dispatchIndirectFeatureSet;

    /// Feature testing, bitset
    uint64_t dispatchIndirectFeatureSetBitsMany = 0;
    uint64_t dispatchIndirectFeatureSetBitsManyFewEnabled = 0;
    std::vector<IFeature*> dispatchIndirectFeatureSetMany;

private:
    static std::mutex Mutex;
    static std::map<void *, DeviceDispatchTable *> Table;
};

std::mutex                                InstanceDispatchTable::Mutex;
std::map<void *, InstanceDispatchTable *> InstanceDispatchTable::Table;
std::mutex                                DeviceDispatchTable::Mutex;
std::map<void *, DeviceDispatchTable *>   DeviceDispatchTable::Table;

VkResult GetLayerProperties(uint32_t *pPropertyCount, VkLayerProperties *pProperties) {
    if(pPropertyCount) {
        *pPropertyCount = 1;
    }

    if(pProperties) {
        strcpy_s(pProperties->layerName, LAYER_NAME);
        strcpy_s(pProperties->description, "");
        pProperties->implementationVersion = 1;
        pProperties->specVersion = VK_API_VERSION_1_0;
    }

    return VK_SUCCESS;
}

VkResult GetExtensionProperties(uint32_t *pPropertyCount, VkExtensionProperties *pProperties) {
    if(pPropertyCount) {
        *pPropertyCount = 0;
    }

    return VK_SUCCESS;
}

VkResult VKAPI_PTR CreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) {
    // Attempt to find link info
    auto chainInfo = (VkLayerInstanceCreateInfo *) pCreateInfo->pNext;
    while (chainInfo && !(chainInfo->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO && chainInfo->function == VK_LAYER_LINK_INFO)) {
        chainInfo = (VkLayerInstanceCreateInfo *) chainInfo->pNext;
    }

    // ...
    if (!chainInfo) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Fetch previous addresses
    PFN_vkGetInstanceProcAddr getInstanceProcAddr = chainInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;

    // Advance layer
    chainInfo->u.pLayerInfo = chainInfo->u.pLayerInfo->pNext;

    // Pass down the chain
    VkResult result = reinterpret_cast<PFN_vkCreateInstance>(getInstanceProcAddr(nullptr, "vkCreateInstance"))(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Insert dispatch table
    auto table = new InstanceDispatchTable{};
    table->getInstanceProcAddr = getInstanceProcAddr;
    table->destroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(getInstanceProcAddr(*pInstance, "vkDestroyInstance"));
    table->enumerateDeviceExtensionProperties = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(getInstanceProcAddr(*pInstance, "vkEnumerateDeviceExtensionProperties"));
    InstanceDispatchTable::Add(GetInternalTable(*pInstance), table);

    return VK_SUCCESS;
}

void VKAPI_PTR DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    auto table = InstanceDispatchTable::Get(GetInternalTable(instance));

    // Pass down callchain
    table->destroyInstance(instance, pAllocator);

    // Release the table
    delete table;
}

VKAPI_ATTR void VKAPI_CALL CmdDispatchNull(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {

}

VKAPI_ATTR void VKAPI_CALL CmdDispatchIndirectNull(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) {

}

VkResult VKAPI_PTR CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    // Attempt to find link info
    auto chainInfo = (VkLayerDeviceCreateInfo *) pCreateInfo->pNext;
    while (chainInfo && !(chainInfo->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO && chainInfo->function == VK_LAYER_LINK_INFO)) {
        chainInfo = (VkLayerDeviceCreateInfo *) chainInfo->pNext;
    }

    // ...
    if (!chainInfo) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Fetch previous addresses
    PFN_vkGetInstanceProcAddr getInstanceProcAddr = chainInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr   getDeviceProcAddr   = chainInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr;

    // Pass down the chain
    VkResult result = reinterpret_cast<PFN_vkCreateDevice>(getInstanceProcAddr(nullptr, "vkCreateDevice"))(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Insert dispatch table
    auto table = new DeviceDispatchTable{};
    table->device                   = *pDevice;
    table->getDeviceProcAddr        = getDeviceProcAddr;
    table->destroyDevice            = reinterpret_cast<PFN_vkDestroyDevice>(getDeviceProcAddr(*pDevice, "vkDestroyDevice"));
    table->cmdBindPipeline          = reinterpret_cast<PFN_vkCmdBindPipeline>(getDeviceProcAddr(*pDevice, "vkCmdBindPipeline"));
    table->cmdDispatch              = CmdDispatchNull;
    table->cmdDispatchIndirect      = CmdDispatchIndirectNull;
    table->createPrivateDataSlotEXT = reinterpret_cast<PFN_vkCreatePrivateDataSlotEXT>(getDeviceProcAddr(*pDevice, "vkCreatePrivateDataSlotEXT"));
    table->destroyPrivateDataSlotEXT = reinterpret_cast<PFN_vkDestroyPrivateDataSlotEXT>(getDeviceProcAddr(*pDevice, "vkDestroyPrivateDataSlotEXT"));
    table->setPrivateDataEXT        = reinterpret_cast<PFN_vkSetPrivateDataEXT>(getDeviceProcAddr(*pDevice, "vkSetPrivateDataEXT"));
    table->getPrivateDataEXT        = reinterpret_cast<PFN_vkGetPrivateDataEXT>(getDeviceProcAddr(*pDevice, "vkGetPrivateDataEXT"));
    table->allocateCommandBuffers   = reinterpret_cast<PFN_vkAllocateCommandBuffers>(getDeviceProcAddr(*pDevice, "vkAllocateCommandBuffers"));
    table->freeCommandBuffers       = reinterpret_cast<PFN_vkFreeCommandBuffers>(getDeviceProcAddr(*pDevice, "vkFreeCommandBuffers"));
    table->beginCommandBuffer       = reinterpret_cast<PFN_vkBeginCommandBuffer>(getDeviceProcAddr(*pDevice, "vkBeginCommandBuffer"));
    table->endCommandBuffer         = reinterpret_cast<PFN_vkEndCommandBuffer>(getDeviceProcAddr(*pDevice, "vkEndCommandBuffer"));
    DeviceDispatchTable::Add(GetInternalTable(*pDevice), table);

    // Feature setup
    {
        table->feature = new FeatureA();

        table->dispatchIndirectFeatures.push_back(table->feature);

        table->dispatchIndirectFeaturesFlat[0] = table->feature;
        table->dispatchIndirectFeaturesFlatCount = 1;

        table->dispatchIndirectFeatureSet.resize(20);
        table->dispatchIndirectFeatureSet[0] = table->feature;
        table->dispatchIndirectFeatureSetBits = (1 << 0);

        table->dispatchIndirectFeatureSetMany.resize(20);
        table->dispatchIndirectFeatureSetBitsMany = 0;

        for (uint32_t i = 0; i < 20; i++) {
            if (i % 3 == 0)
                continue;

            table->dispatchIndirectFeatureSetMany[i] = table->feature;
            table->dispatchIndirectFeatureSetBitsMany |= (1ull << i);
        }

        table->dispatchIndirectFeatureSetBitsManyFewEnabled |= (1 << 4);
    }

    // Allocate private slot if possible
    if (table->createPrivateDataSlotEXT)
    {
        VkPrivateDataSlotCreateInfoEXT privateInfo{};
        privateInfo.sType = VK_STRUCTURE_TYPE_PRIVATE_DATA_SLOT_CREATE_INFO_EXT;

        result = table->createPrivateDataSlotEXT(*pDevice, &privateInfo, nullptr, &table->slot);
        if (result != VK_SUCCESS) {
            return result;
        }
    }

    // OK
    return VK_SUCCESS;
}

void VKAPI_PTR DestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    auto table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Destroy private data
    table->destroyPrivateDataSlotEXT(device, table->slot, pAllocator);

    // Pass down callchain
    table->destroyDevice(device, pAllocator);

    // Release the feature
    delete table->feature;

    // Release the table
    delete table;
}

struct WrappedCommandBuffer
{
    void*                dispatchTable;
    DeviceDispatchTable* table;
    VkCommandBuffer      object;
};

VKAPI_ATTR void VKAPI_CALL CmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
    auto wrapped = reinterpret_cast<WrappedCommandBuffer*>(commandBuffer);
    wrapped->table->cmdBindPipeline(wrapped->object, pipelineBindPoint, pipeline);
}

VKAPI_ATTR void VKAPI_CALL CmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    auto wrapped = reinterpret_cast<WrappedCommandBuffer*>(commandBuffer);

    // Select implementation based on the dispatch count
    //  ? Slightly arbitrary, but works quite well
    switch (groupCountX) {
        case 0: {
            // Fetch from lookup  table
            DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(commandBuffer));
            table->cmdDispatch(wrapped->object, 16, groupCountY, groupCountZ);
            break;
        }
        case 1: {
            // Wrapped
            wrapped->table->cmdDispatch(wrapped->object, 16, groupCountY, groupCountZ);
            break;
        }
        case 2: {
            // Private
            uint64_t data;
            wrapped->table->getPrivateDataEXT(wrapped->table->device, VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64_t>(wrapped->object), wrapped->table->slot, &data);

            reinterpret_cast<DeviceDispatchTable*>(data)->cmdDispatch(wrapped->object, 16, groupCountY, groupCountZ);
            break;
        }
    }
}

VKAPI_ATTR void VKAPI_CALL CmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) {
    auto wrapped = reinterpret_cast<WrappedCommandBuffer*>(commandBuffer);

    // Select implementation based on the offset
    //  ? Slightly arbitrary, but works quite well
    switch (offset) {
        case 0: {
            // Pass through, for baseline speeds
            wrapped->table->cmdDispatchIndirect(wrapped->object, buffer, offset);
            break;
        }
        case 1: {
            // Std vector, linear
            for (IFeature* feature : wrapped->table->dispatchIndirectFeatures)
            {
                feature->Foo();
            }

            wrapped->table->cmdDispatchIndirect(wrapped->object, buffer, offset);
            break;
        }
        case 2: {
            // Flat
            for (uint32_t i= 0; i < wrapped->table->dispatchIndirectFeaturesFlatCount; i++)
            {
                wrapped->table->dispatchIndirectFeaturesFlat[i]->Foo();
            }

            wrapped->table->cmdDispatchIndirect(wrapped->object, buffer, offset);
            break;
        }
        case 3: {
            // Vector, if 0 basically
            if (wrapped->table->dispatchIndirectFeatureSetZero)
            {
                for (uint32_t i= 0; i < wrapped->table->dispatchIndirectFeaturesFlatCount; i++)
                {
                    wrapped->table->dispatchIndirectFeaturesFlat[i]->Foo();
                }
            }

            wrapped->table->cmdDispatchIndirect(wrapped->object, buffer, offset);
            break;
        }
        case 4: {
            // Vector, bit loop
            if (wrapped->table->dispatchIndirectFeatureSetBits)
            {
                uint64_t bitMask = wrapped->table->dispatchIndirectFeatureSetBits;

                unsigned long index;
                while (_BitScanReverse64(&index, bitMask))
                {
                    wrapped->table->dispatchIndirectFeatureSet[index]->Foo();
                    bitMask &= ~(1ull << index);
                }
            }

            wrapped->table->cmdDispatchIndirect(wrapped->object, buffer, offset);
            break;
        }
        case 5: {
            // Vector, many features, null checks
            for (uint32_t i = 0; i < wrapped->table->dispatchIndirectFeatureSetMany.size(); i++)
            {
                if (!wrapped->table->dispatchIndirectFeatureSetMany[i])
                    continue;

                wrapped->table->dispatchIndirectFeatureSetMany[i]->Foo();
            }

            wrapped->table->cmdDispatchIndirect(wrapped->object, buffer, offset);
            break;
        }
        case 6: {
            // Vector, many features, bit loop
            if (wrapped->table->dispatchIndirectFeatureSetBitsMany)
            {
                uint64_t bitMask = wrapped->table->dispatchIndirectFeatureSetBitsMany;

                unsigned long index;
                while (_BitScanReverse64(&index, bitMask))
                {
                    wrapped->table->dispatchIndirectFeatureSetMany[index]->Foo();
                    bitMask &= ~(1ull << index);
                }
            }

            wrapped->table->cmdDispatchIndirect(wrapped->object, buffer, offset);
            break;
        }
        case 7: {
            // Vector, many features, few enabled, bit loop
            if (wrapped->table->dispatchIndirectFeatureSetBitsManyFewEnabled)
            {
                uint64_t bitMask = wrapped->table->dispatchIndirectFeatureSetBitsManyFewEnabled;

                unsigned long index;
                while (_BitScanReverse64(&index, bitMask))
                {
                    wrapped->table->dispatchIndirectFeatureSetMany[index]->Foo();
                    bitMask &= ~(1ull << index);
                }
            }

            wrapped->table->cmdDispatchIndirect(wrapped->object, buffer, offset);
            break;
        }
        case 8: {
            // Vector, many features, virtuals
            for (uint32_t i = 0; i < wrapped->table->dispatchIndirectFeatureSetMany.size(); i++)
            {
                if (!wrapped->table->dispatchIndirectFeatureSetMany[i])
                    continue;

                wrapped->table->dispatchIndirectFeatureSetMany[i]->Foo();
            }

            wrapped->table->cmdDispatchIndirect(wrapped->object, buffer, offset);
            break;
        }
        case 9: {
            // Vector, many features, fptrs
            for (uint32_t i = 0; i < wrapped->table->dispatchIndirectFeatureSetMany.size(); i++)
            {
                if (!wrapped->table->dispatchIndirectFeatureSetMany[i])
                    continue;

                wrapped->table->dispatchIndirectFeatureSetMany[i]->fooFPtr(wrapped->table->dispatchIndirectFeatureSetMany[i]);
            }

            wrapped->table->cmdDispatchIndirect(wrapped->object, buffer, offset);
            break;
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo, VkCommandBuffer *pCommandBuffers) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->allocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Wrap objects
    for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
        if (table->setPrivateDataEXT) {
            table->setPrivateDataEXT(device, VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64_t>(pCommandBuffers[i]), table->slot, reinterpret_cast<uint64_t>(table));
        }

        auto wrapped = new WrappedCommandBuffer;
        wrapped->dispatchTable = GetInternalTable(pCommandBuffers[i]);
        wrapped->object = pCommandBuffers[i];
        wrapped->table = table;
        pCommandBuffers[i] = reinterpret_cast<VkCommandBuffer>(wrapped);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL FreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Unwrap and release wrappers
    auto unwrappedBuffers = static_cast<VkCommandBuffer*>(alloca(sizeof(VkCommandBuffer) * commandBufferCount));
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        auto wrapped = reinterpret_cast<WrappedCommandBuffer*>(pCommandBuffers[i]);
        unwrappedBuffers[i] =  wrapped->object;
        delete wrapped;
    }

    // Pass down callchain
    table->freeCommandBuffers(device, commandPool, commandBufferCount, unwrappedBuffers);
}

VKAPI_ATTR VkResult VKAPI_CALL BeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo) {
    auto wrapped = reinterpret_cast<WrappedCommandBuffer*>(commandBuffer);

    // Pass down callchain
    return wrapped->table->beginCommandBuffer(wrapped->object, pBeginInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL EndCommandBuffer(VkCommandBuffer commandBuffer) {
    auto wrapped = reinterpret_cast<WrappedCommandBuffer*>(commandBuffer);

    // Pass down callchain
    return wrapped->table->endCommandBuffer(wrapped->object);
}

static VKAPI_ATTR PFN_vkVoidFunction GetSharedProcAddr(void *, const char *pName) {
    if (!std::strcmp(pName, "vkCreateDevice")) {
        return reinterpret_cast<PFN_vkVoidFunction>(&CreateDevice);
    }

    if (!std::strcmp(pName, "vkDestroyDevice")) {
        return reinterpret_cast<PFN_vkVoidFunction>(&DestroyDevice);
    }

    if (!std::strcmp(pName, "vkAllocateCommandBuffers")) {
        return reinterpret_cast<PFN_vkVoidFunction>(&AllocateCommandBuffers);
    }

    if (!std::strcmp(pName, "vkFreeCommandBuffers")) {
        return reinterpret_cast<PFN_vkVoidFunction>(&FreeCommandBuffers);
    }

    if (!std::strcmp(pName, "vkBeginCommandBuffer")) {
        return reinterpret_cast<PFN_vkVoidFunction>(&BeginCommandBuffer);
    }

    if (!std::strcmp(pName, "vkEndCommandBuffer")) {
        return reinterpret_cast<PFN_vkVoidFunction>(&EndCommandBuffer);
    }

    if (!std::strcmp(pName, "vkCmdDispatch")) {
        return reinterpret_cast<PFN_vkVoidFunction>(&CmdDispatch);
    }

    if (!std::strcmp(pName, "vkCmdDispatchIndirect")) {
        return reinterpret_cast<PFN_vkVoidFunction>(&CmdDispatchIndirect);
    }

    if (!std::strcmp(pName, "vkCmdBindPipeline")) {
        return reinterpret_cast<PFN_vkVoidFunction>(&CmdBindPipeline);
    }

    return nullptr;
}

// All exports
extern "C" {
    EXPORT_C VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL Hook_vkGetDeviceProcAddr(VkDevice device, const char *pName) {
        if (!std::strcmp(pName, "vkGetDeviceProcAddr")) {
            return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkGetDeviceProcAddr);
        }

        if (!std::strcmp(pName, "vkEnumerateDeviceLayerProperties")) {
            return reinterpret_cast<PFN_vkVoidFunction>(&GetLayerProperties);
        }

        if (!std::strcmp(pName, "vkEnumerateDeviceExtensionProperties")) {
            return reinterpret_cast<PFN_vkVoidFunction>(&GetExtensionProperties);
        }

        if (PFN_vkVoidFunction addr = GetSharedProcAddr(device, pName)) {
            return addr;
        }

        return DeviceDispatchTable::Get(GetInternalTable(device))->getDeviceProcAddr(device, pName);
    }

    EXPORT_C VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL Hook_vkGetInstanceProcAddr(VkInstance instance, const char *pName) {
        if (!std::strcmp(pName, "vkGetDeviceProcAddr")) {
            return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkGetInstanceProcAddr);
        }

        if (!std::strcmp(pName, "vkGetDeviceProcAddr")) {
            return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkGetDeviceProcAddr);
        }

        if (!std::strcmp(pName, "vkCreateInstance")) {
            return reinterpret_cast<PFN_vkVoidFunction>(&CreateInstance);
        }

        if (!std::strcmp(pName, "vkDestroyInstance")) {
            return reinterpret_cast<PFN_vkVoidFunction>(&DestroyInstance);
        }

        if (!std::strcmp(pName, "vkEnumerateInstanceLayerProperties")) {
            return reinterpret_cast<PFN_vkVoidFunction>(&GetLayerProperties);
        }

        if (!std::strcmp(pName, "vkEnumerateInstanceExtensionProperties")) {
            return reinterpret_cast<PFN_vkVoidFunction>(&GetExtensionProperties);
        }

        if (PFN_vkVoidFunction addr = GetSharedProcAddr(instance, pName)) {
            return addr;
        }

        return InstanceDispatchTable::Get(GetInternalTable(instance))->getInstanceProcAddr(instance, pName);
    }

    [[maybe_unused]] EXPORT_C VKAPI_ATTR VkResult VKAPI_CALL Hook_vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface *pVersionStruct) {
        if (pVersionStruct->loaderLayerInterfaceVersion >= 2) {
            pVersionStruct->pfnGetInstanceProcAddr = Hook_vkGetInstanceProcAddr;
            pVersionStruct->pfnGetDeviceProcAddr = Hook_vkGetDeviceProcAddr;
            pVersionStruct->pfnGetPhysicalDeviceProcAddr = nullptr;
        }

        if (pVersionStruct->loaderLayerInterfaceVersion > CURRENT_LOADER_LAYER_INTERFACE_VERSION) {
            pVersionStruct->loaderLayerInterfaceVersion = CURRENT_LOADER_LAYER_INTERFACE_VERSION;
        }

        return VK_SUCCESS;
    }
}