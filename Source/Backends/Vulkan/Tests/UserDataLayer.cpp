
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
#define LAYER_NAME "UserDataLayer"

/// Linkage information
#if defined(_WIN32)
#	define EXPORT_C extern "C" __declspec(dllexport)
#else
#	define EXPORT_C extern "C" __attribute__((visibility("default")))
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

    PFN_vkGetInstanceProcAddr                GetInstanceProcAddr;
    PFN_vkEnumerateDeviceExtensionProperties EnumerateDeviceExtensionProperties;

private:
    static std::mutex Mutex;
    static std::map<void *, InstanceDispatchTable *> Table;
};

class IFeature
{
public:
    virtual void Foo() = 0;

    // Function pointer test
    void(*FooFPtr)(IFeature* frame){nullptr};
};

class FeatureA final : public IFeature
{
public:
    FeatureA()
    {
        FooFPtr = [](IFeature* frame)
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
    PFN_vkCmdBindPipeline          CmdBindPipeline;
    PFN_vkCreatePrivateDataSlotEXT CreatePrivateDataSlotEXT;
    PFN_vkSetPrivateDataEXT        SetPrivateDataEXT;
    PFN_vkGetPrivateDataEXT        GetPrivateDataEXT;
    PFN_vkCmdDispatch              CmdDispatch;
    PFN_vkCmdDispatchIndirect      CmdDispatchIndirect;
    PFN_vkAllocateCommandBuffers   AllocateCommandBuffers;
    PFN_vkBeginCommandBuffer       BeginCommandBuffer;
    PFN_vkEndCommandBuffer         EndCommandBuffer;
    PFN_vkGetDeviceProcAddr        GetDeviceProcAddr;

    /// Data
    VkDevice             Device;
    VkPrivateDataSlotEXT Slot;

    /// Feature testing, vector
    std::vector<IFeature*> DispatchIndirectFeatures;

    /// Feature testing, inline
    IFeature* DispatchIndirectFeaturesFlat[1];
    uint32_t DispatchIndirectFeaturesFlatCount;

    /// Feature testing, bitset
    uint64_t DispatchIndirectFeatureSetZero = 0;

    /// Feature testing, bitset
    uint64_t DispatchIndirectFeatureSetBits = 0;
    std::vector<IFeature*> DispatchIndirectFeatureSet;

    /// Feature testing, bitset
    uint64_t DispatchIndirectFeatureSetBitsMany = 0;
    uint64_t DispatchIndirectFeatureSetBitsManyFewEnabled = 0;
    std::vector<IFeature*> DispatchIndirectFeatureSetMany;

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
        strcpy(pProperties->layerName, LAYER_NAME);
        strcpy(pProperties->description, "");
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
    auto chain_info = (VkLayerInstanceCreateInfo *) pCreateInfo->pNext;
    while (chain_info && !(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO && chain_info->function == VK_LAYER_LINK_INFO)) {
        chain_info = (VkLayerInstanceCreateInfo *) chain_info->pNext;
    }

    // ...
    if (!chain_info) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Fetch previous addresses
    PFN_vkGetInstanceProcAddr getInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;

    // Advance layer
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    // Pass down the chain
    VkResult result = reinterpret_cast<PFN_vkCreateInstance>(getInstanceProcAddr(nullptr, "vkCreateInstance"))(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Insert dispatch table
    auto table = new InstanceDispatchTable{};
    table->GetInstanceProcAddr = getInstanceProcAddr;
    table->EnumerateDeviceExtensionProperties = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(getInstanceProcAddr(*pInstance, "vkEnumerateDeviceExtensionProperties"));
    InstanceDispatchTable::Add(GetInternalTable(*pInstance), table);

    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL CmdDispatchNull(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {

}

VKAPI_ATTR void VKAPI_CALL CmdDispatchIndirectNull(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) {

}

VkResult VKAPI_PTR CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    // Attempt to find link info
    auto chain_info = (VkLayerDeviceCreateInfo *) pCreateInfo->pNext;
    while (chain_info && !(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO && chain_info->function == VK_LAYER_LINK_INFO)) {
        chain_info = (VkLayerDeviceCreateInfo *) chain_info->pNext;
    }

    // ...
    if (!chain_info) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Fetch previous addresses
    PFN_vkGetInstanceProcAddr getInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr   getDeviceProcAddr   = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;

    // Pass down the chain
    VkResult result = reinterpret_cast<PFN_vkCreateDevice>(getInstanceProcAddr(nullptr, "vkCreateDevice"))(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Insert dispatch table
    auto table = new DeviceDispatchTable{};
    table->Device                   = *pDevice;
    table->GetDeviceProcAddr        = getDeviceProcAddr;
    table->CmdBindPipeline          = reinterpret_cast<PFN_vkCmdBindPipeline>(getDeviceProcAddr(*pDevice, "vkCmdBindPipeline"));
    table->CmdDispatch              = CmdDispatchNull;
    table->CmdDispatchIndirect      = CmdDispatchIndirectNull;
    table->CreatePrivateDataSlotEXT = reinterpret_cast<PFN_vkCreatePrivateDataSlotEXT>(getDeviceProcAddr(*pDevice, "vkCreatePrivateDataSlotEXT"));
    table->SetPrivateDataEXT        = reinterpret_cast<PFN_vkSetPrivateDataEXT>(getDeviceProcAddr(*pDevice, "vkSetPrivateDataEXT"));
    table->GetPrivateDataEXT        = reinterpret_cast<PFN_vkGetPrivateDataEXT>(getDeviceProcAddr(*pDevice, "vkGetPrivateDataEXT"));
    table->AllocateCommandBuffers   = reinterpret_cast<PFN_vkAllocateCommandBuffers>(getDeviceProcAddr(*pDevice, "vkAllocateCommandBuffers"));
    table->BeginCommandBuffer       = reinterpret_cast<PFN_vkBeginCommandBuffer>(getDeviceProcAddr(*pDevice, "vkBeginCommandBuffer"));
    table->EndCommandBuffer         = reinterpret_cast<PFN_vkEndCommandBuffer>(getDeviceProcAddr(*pDevice, "vkEndCommandBuffer"));
    DeviceDispatchTable::Add(GetInternalTable(*pDevice), table);

    // Feature setup
    {
        auto* feature = new FeatureA();

        table->DispatchIndirectFeatures.push_back(feature);

        table->DispatchIndirectFeaturesFlat[0] = feature;
        table->DispatchIndirectFeaturesFlatCount = 1;

        table->DispatchIndirectFeatureSet.resize(20);
        table->DispatchIndirectFeatureSet[0] = feature;
        table->DispatchIndirectFeatureSetBits = (1 << 0);

        table->DispatchIndirectFeatureSetMany.resize(20);
        table->DispatchIndirectFeatureSetBitsMany = 0;

        for (uint32_t i = 0; i < 20; i++) {
            if (i % 3 == 0)
                continue;

            table->DispatchIndirectFeatureSetMany[i] = feature;
            table->DispatchIndirectFeatureSetBitsMany |= (1 << i);
        }

        table->DispatchIndirectFeatureSetBitsManyFewEnabled |= (1 << 4);
    }

    // Allocate private slot if possible
    if (table->CreatePrivateDataSlotEXT)
    {
        VkPrivateDataSlotCreateInfoEXT privateInfo{};
        privateInfo.sType = VK_STRUCTURE_TYPE_PRIVATE_DATA_SLOT_CREATE_INFO_EXT;

        result = table->CreatePrivateDataSlotEXT(*pDevice, &privateInfo, nullptr, &table->Slot);
        if (result != VK_SUCCESS) {
            return result;
        }
    }

    // OK
    return VK_SUCCESS;
}

struct WrappedCommandBuffer
{
    void*                DispatchTable;
    DeviceDispatchTable* Table;
    VkCommandBuffer      Object;
};

VKAPI_ATTR void VKAPI_CALL CmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
    auto wrapped = reinterpret_cast<WrappedCommandBuffer*>(commandBuffer);
    wrapped->Table->CmdBindPipeline(wrapped->Object, pipelineBindPoint, pipeline);
}

VKAPI_ATTR void VKAPI_CALL CmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    auto wrapped = reinterpret_cast<WrappedCommandBuffer*>(commandBuffer);

    // Select implementation based on the dispatch count
    //  ? Slightly arbitrary, but works quite well
    switch (groupCountX) {
        case 0: {
            // Fetch from lookup  table
            DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(commandBuffer));
            table->CmdDispatch(wrapped->Object, 16, groupCountY, groupCountZ);
            break;
        }
        case 1: {
            // Wrapped
            wrapped->Table->CmdDispatch(wrapped->Object, 16, groupCountY, groupCountZ);
            break;
        }
        case 2: {
            // Private
            uint64_t data;
            wrapped->Table->GetPrivateDataEXT(wrapped->Table->Device, VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64_t>(wrapped->Object), wrapped->Table->Slot, &data);

            reinterpret_cast<DeviceDispatchTable*>(data)->CmdDispatch(wrapped->Object, 16, groupCountY, groupCountZ);
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
            wrapped->Table->CmdDispatchIndirect(wrapped->Object, buffer, offset);
            break;
        }
        case 1: {
            // Std vector, linear
            for (IFeature* feature : wrapped->Table->DispatchIndirectFeatures)
            {
                feature->Foo();
            }

            wrapped->Table->CmdDispatchIndirect(wrapped->Object, buffer, offset);
            break;
        }
        case 2: {
            // Flat
            for (uint32_t i= 0; i < wrapped->Table->DispatchIndirectFeaturesFlatCount; i++)
            {
                wrapped->Table->DispatchIndirectFeaturesFlat[i]->Foo();
            }

            wrapped->Table->CmdDispatchIndirect(wrapped->Object, buffer, offset);
            break;
        }
        case 3: {
            // Vector, if 0 basically
            if (wrapped->Table->DispatchIndirectFeatureSetZero)
            {
                for (uint32_t i= 0; i < wrapped->Table->DispatchIndirectFeaturesFlatCount; i++)
                {
                    wrapped->Table->DispatchIndirectFeaturesFlat[i]->Foo();
                }
            }

            wrapped->Table->CmdDispatchIndirect(wrapped->Object, buffer, offset);
            break;
        }
        case 4: {
            // Vector, bit loop
            if (wrapped->Table->DispatchIndirectFeatureSetBits)
            {
                unsigned long bitMask = wrapped->Table->DispatchIndirectFeatureSetBits;

                unsigned long index;
                while (_BitScanReverse(&index, bitMask))
                {
                    wrapped->Table->DispatchIndirectFeatureSet[index]->Foo();
                    bitMask &= ~(1ul << index);
                }
            }

            wrapped->Table->CmdDispatchIndirect(wrapped->Object, buffer, offset);
            break;
        }
        case 5: {
            // Vector, many features, null checks
            for (uint32_t i = 0; i < wrapped->Table->DispatchIndirectFeatureSetMany.size(); i++)
            {
                if (!wrapped->Table->DispatchIndirectFeatureSetMany[i])
                    continue;

                wrapped->Table->DispatchIndirectFeatureSetMany[i]->Foo();
            }

            wrapped->Table->CmdDispatchIndirect(wrapped->Object, buffer, offset);
            break;
        }
        case 6: {
            // Vector, many features, bit loop
            if (wrapped->Table->DispatchIndirectFeatureSetBitsMany)
            {
                unsigned long bitMask = wrapped->Table->DispatchIndirectFeatureSetBitsMany;

                unsigned long index;
                while (_BitScanReverse(&index, bitMask))
                {
                    wrapped->Table->DispatchIndirectFeatureSetMany[index]->Foo();
                    bitMask &= ~(1ul << index);
                }
            }

            wrapped->Table->CmdDispatchIndirect(wrapped->Object, buffer, offset);
            break;
        }
        case 7: {
            // Vector, many features, few enabled, bit loop
            if (wrapped->Table->DispatchIndirectFeatureSetBitsManyFewEnabled)
            {
                unsigned long bitMask = wrapped->Table->DispatchIndirectFeatureSetBitsManyFewEnabled;

                unsigned long index;
                while (_BitScanReverse(&index, bitMask))
                {
                    wrapped->Table->DispatchIndirectFeatureSetMany[index]->Foo();
                    bitMask &= ~(1ul << index);
                }
            }

            wrapped->Table->CmdDispatchIndirect(wrapped->Object, buffer, offset);
            break;
        }
        case 8: {
            // Vector, many features, virtuals
            for (uint32_t i = 0; i < wrapped->Table->DispatchIndirectFeatureSetMany.size(); i++)
            {
                if (!wrapped->Table->DispatchIndirectFeatureSetMany[i])
                    continue;

                wrapped->Table->DispatchIndirectFeatureSetMany[i]->Foo();
            }

            wrapped->Table->CmdDispatchIndirect(wrapped->Object, buffer, offset);
            break;
        }
        case 9: {
            // Vector, many features, fptrs
            for (uint32_t i = 0; i < wrapped->Table->DispatchIndirectFeatureSetMany.size(); i++)
            {
                if (!wrapped->Table->DispatchIndirectFeatureSetMany[i])
                    continue;

                wrapped->Table->DispatchIndirectFeatureSetMany[i]->FooFPtr(wrapped->Table->DispatchIndirectFeatureSetMany[i]);
            }

            wrapped->Table->CmdDispatchIndirect(wrapped->Object, buffer, offset);
            break;
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo, VkCommandBuffer *pCommandBuffers) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->AllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Wrap objects
    for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
        if (table->SetPrivateDataEXT) {
            table->SetPrivateDataEXT(device, VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64_t>(pCommandBuffers[i]), table->Slot, reinterpret_cast<uint64_t>(table));
        }

        auto wrapped = new WrappedCommandBuffer;
        wrapped->DispatchTable = GetInternalTable(pCommandBuffers[i]);
        wrapped->Object = pCommandBuffers[i];
        wrapped->Table = table;
        pCommandBuffers[i] = reinterpret_cast<VkCommandBuffer>(wrapped);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL BeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo) {
    auto wrapped = reinterpret_cast<WrappedCommandBuffer*>(commandBuffer);

    // Pass down callchain
    return wrapped->Table->BeginCommandBuffer(wrapped->Object, pBeginInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL EndCommandBuffer(VkCommandBuffer commandBuffer) {
    auto wrapped = reinterpret_cast<WrappedCommandBuffer*>(commandBuffer);

    // Pass down callchain
    return wrapped->Table->EndCommandBuffer(wrapped->Object);
}

static VKAPI_ATTR PFN_vkVoidFunction GetSharedProcAddr(void *, const char *pName) {
    if (!std::strcmp(pName, "vkCreateDevice")) {
        return reinterpret_cast<PFN_vkVoidFunction>(&CreateDevice);
    }

    if (!std::strcmp(pName, "vkAllocateCommandBuffers")) {
        return reinterpret_cast<PFN_vkVoidFunction>(&AllocateCommandBuffers);
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

EXPORT_C VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char *pName) {
    if (!std::strcmp(pName, "vkGetDeviceProcAddr")) {
        return reinterpret_cast<PFN_vkVoidFunction>(&vkGetDeviceProcAddr);
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

    return DeviceDispatchTable::Get(GetInternalTable(device))->GetDeviceProcAddr(device, pName);
}

EXPORT_C VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *pName) {
    if (!std::strcmp(pName, "vkGetDeviceProcAddr")) {
        return reinterpret_cast<PFN_vkVoidFunction>(&vkGetInstanceProcAddr);
    }

    if (!std::strcmp(pName, "vkGetDeviceProcAddr")) {
        return reinterpret_cast<PFN_vkVoidFunction>(&vkGetDeviceProcAddr);
    }

    if (!std::strcmp(pName, "vkCreateInstance")) {
        return reinterpret_cast<PFN_vkVoidFunction>(&CreateInstance);
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

    return InstanceDispatchTable::Get(GetInternalTable(instance))->GetInstanceProcAddr(instance, pName);
}

EXPORT_C VKAPI_ATTR VkResult VKAPI_CALL vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface *pVersionStruct) {
    if (pVersionStruct->loaderLayerInterfaceVersion >= 2) {
        pVersionStruct->pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
        pVersionStruct->pfnGetDeviceProcAddr = vkGetDeviceProcAddr;
        pVersionStruct->pfnGetPhysicalDeviceProcAddr = nullptr;
    }

    if (pVersionStruct->loaderLayerInterfaceVersion > CURRENT_LOADER_LAYER_INTERFACE_VERSION) {
        pVersionStruct->loaderLayerInterfaceVersion = CURRENT_LOADER_LAYER_INTERFACE_VERSION;
    }

    return VK_SUCCESS;
}