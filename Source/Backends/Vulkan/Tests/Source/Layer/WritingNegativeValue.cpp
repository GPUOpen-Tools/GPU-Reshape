// Catch2
#include <catch2/catch.hpp>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStream.h>

// Schemas
#include <Schemas/Pipeline.h>
#include <Schemas/Config.h>
#include <Schemas/WritingNegativeValue.h>
#include <Schemas/SGUID.h>

// Tests
#include <Loader.h>

// Common
#include <Common/String.h>
#include <Common/ComponentTemplate.h>

// Backend
#include <Backend/FeatureHost.h>
#include <Backend/IFeature.h>
#include <Backend/IShaderFeature.h>
#include <Backend/IL/Program.h>
#include <Backend/IL/Emitter.h>
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/ShaderSGUIDHostListener.h>

// Bridge
#include <Bridge/IBridgeListener.h>

// VMA
#include <VMA/vk_mem_alloc.h>

// HLSL
#include <Data/WriteUAVNegativeVulkan.h>

/// Dummy value for validation
static constexpr uint32_t kProxy = 'p' + 'r' + 'o' + 'x' + 'y';

class WritingNegativeValueFeature : public IFeature, public IShaderFeature {
public:
    COMPONENT(WritingNegativeValueFeature);

    bool Install() override {
        auto exportHost = registry->Get<IShaderExportHost>();
        exportID = exportHost->Allocate<WritingNegativeValueMessage>();

        guidHost = registry->Get<IShaderSGUIDHost>();
        return true;
    }

    FeatureHookTable GetHookTable() override {
        return FeatureHookTable{};
    }

    void CollectExports(const MessageStream &exports) override {
        stream.Append(exports);
    }

    void CollectMessages(IMessageStorage *storage) override {
        storage->AddStreamAndSwap(stream);
    }

    void Inject(IL::Program &program) override {
        for (IL::Function* fn : program.GetFunctionList()) {
            for (IL::BasicBlock* bb : fn->GetBasicBlocks()) {
                for (auto it = bb->begin(); it != bb->end(); ++it) {
                    if (Instrument(program, fn, bb, it)) {
                        return;
                    }
                }
            }
        }
    }

    bool Instrument(IL::Program &program, IL::Function* fn, IL::BasicBlock* bb, const IL::BasicBlock::Iterator& it) {
        switch (it->opCode) {
            default:
                return false;

            case IL::OpCode::StoreBuffer: {
                ShaderSGUID sguid = guidHost->Bind(program, it);

                // Pre:
                //   BrCond Fail Resume
                // Fail:
                //   ExportMessage
                // Resume:
                //   StoreBuffer
                IL::BasicBlock* resumeBlock = fn->GetBasicBlocks().AllocBlock();

                // Split this basic block
                auto storeBuffer = bb->Split<IL::StoreBufferInstruction>(resumeBlock, it);

                // Failure condition
                IL::BasicBlock* failBlock = fn->GetBasicBlocks().AllocBlock();
                {
                    IL::Emitter<> emitter(program, *failBlock);

                    WritingNegativeValueMessage::ShaderExport msg;
                    msg.sguid = emitter.UInt32(sguid);
                    msg.ergo = emitter.UInt32(kProxy);
                    emitter.Export(exportID, msg);

                    // Branch back
                    emitter.Branch(resumeBlock);
                }

                IL::Emitter<> pre(program, *bb);
                pre.BranchConditional(pre.LessThan(storeBuffer->value.GetVector(), pre.Int(32, 0)), failBlock, resumeBlock, IL::ControlFlow::Selection(resumeBlock));
                return true;
            }
        }
    }

    void *QueryInterface(ComponentID id) override {
        switch (id) {
            case IComponent::kID:
                return static_cast<IComponent*>(this);
            case IFeature::kID:
                return static_cast<IFeature*>(this);
            case IShaderFeature::kID:
                return static_cast<IShaderFeature*>(this);
        }

        return nullptr;
    }

private:
    ComRef<IShaderSGUIDHost> guidHost{nullptr};

    ShaderExportID exportID{};

    MessageStream stream;
};

class WritingNegativeValueListener : public TComponent<WritingNegativeValueListener>, public IBridgeListener {
public:
    COMPONENT(WritingNegativeValueListener);

    WritingNegativeValueListener(Registry* registry) {
        sguidHost = registry->Get<ShaderSGUIDHostListener>();
    }

    void Handle(const MessageStream *streams, uint32_t count) override {
        for (uint32_t i = 0; i < count; i++) {
            ConstMessageStreamView<WritingNegativeValueMessage> view(streams[i]);

            // Must have 4 messages (number of cs threads, except the first zero id)
            REQUIRE(view.GetCount() == 3);

            // Validate all messages
            for (auto it = view.GetIterator(); it; ++it) {
                REQUIRE(it->sguid != InvalidShaderSGUID);

                std::string_view line = sguidHost->GetSource(it->sguid);
                REQUIRE(std::trim_copy(std::string(line)) == "output[dtid.x] = -(int)dtid;");

                REQUIRE(it->ergo == kProxy);
            }

            visited = true;
        }
    }

    bool visited{false};

private:
    ComRef<ShaderSGUIDHostListener> sguidHost;
};

TEST_CASE_METHOD(Loader, "Layer.Feature.WritingNegativeValue", "[Vulkan]") {
    REQUIRE(AddInstanceLayer("VK_LAYER_GPUOPEN_GBV"));

    Registry* registry = GetRegistry();

    auto host = registry->Get<IFeatureHost>();
    host->Register(registry->New<ComponentTemplate<WritingNegativeValueFeature>>());

    // Create the instance & device
    CreateInstance();
    CreateDevice();

    VmaAllocator allocator;

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.instance = GetInstance();
    allocatorInfo.physicalDevice = GetPhysicalDevice();
    allocatorInfo.device = GetDevice();
    vmaCreateAllocator(&allocatorInfo, &allocator);

    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = sizeof(int32_t) * 4;
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;

    VkBuffer buffer;
    VmaAllocation allocation;
    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);

    VkBufferViewCreateInfo bufferViewInfo{};
    bufferViewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    bufferViewInfo.buffer = buffer;
    bufferViewInfo.format = VK_FORMAT_R32_SINT;
    bufferViewInfo.range = sizeof(int32_t) * 4;

    VkBufferView bufferView;
    REQUIRE(vkCreateBufferView(GetDevice(), &bufferViewInfo, nullptr, &bufferView) == VK_SUCCESS);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = GetPrimaryQueueFamily();

    // Attempt to create the pool
    VkCommandPool commandPool;
    REQUIRE(vkCreateCommandPool(GetDevice(), &poolInfo, nullptr, &commandPool) == VK_SUCCESS);

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandPool        = commandPool;
    allocateInfo.commandBufferCount = 1;

    // Attempt to allocate the command buffer
    VkCommandBuffer commandBuffer;
    REQUIRE(vkAllocateCommandBuffers(GetDevice(), &allocateInfo, &commandBuffer) == VK_SUCCESS);

    VkMemoryBarrier barrier{};
    barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_HOST_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_WRITE_BIT;

    VkDescriptorSetLayoutBinding bindingInfo{};
    bindingInfo.binding = 0;
    bindingInfo.descriptorCount = 1;
    bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    bindingInfo.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{};
    descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayoutInfo.bindingCount = 1;
    descriptorLayoutInfo.pBindings = &bindingInfo;

    VkDescriptorSetLayout setLayout;
    REQUIRE(vkCreateDescriptorSetLayout(GetDevice(), &descriptorLayoutInfo, nullptr, &setLayout) == VK_SUCCESS);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &setLayout;

    VkPipelineLayout pipelineLayout;
    REQUIRE(vkCreatePipelineLayout(GetDevice(), &layoutInfo, nullptr, &pipelineLayout) == VK_SUCCESS);

    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = sizeof(kSPIRVWriteUAVNegativeVulkan);
    moduleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(kSPIRVWriteUAVNegativeVulkan);

    VkShaderModule shaderModule;
    REQUIRE(vkCreateShaderModule(GetDevice(), &moduleCreateInfo, nullptr, &shaderModule) == VK_SUCCESS);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.pName = "main";
    stageInfo.module = shaderModule;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.stage  = stageInfo;

    VkPipeline pipeline;
    REQUIRE(vkCreateComputePipelines(GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) == VK_SUCCESS);

    VkDescriptorPoolSize poolSize{};
    poolSize.descriptorCount = 1;
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;

    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType  = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.pPoolSizes = &poolSize;
    descriptorPoolInfo.poolSizeCount = 1;
    descriptorPoolInfo.maxSets = 1;

    VkDescriptorPool descriptorPool;
    REQUIRE(vkCreateDescriptorPool(GetDevice(), &descriptorPoolInfo, nullptr, &descriptorPool) == VK_SUCCESS);

    VkDescriptorSetAllocateInfo setInfo{};
    setInfo.sType  = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setInfo.pSetLayouts = &setLayout;
    setInfo.descriptorPool = descriptorPool;
    setInfo.descriptorSetCount = 1;

    VkDescriptorSet set;
    REQUIRE(vkAllocateDescriptorSets(GetDevice(), &setInfo, &set) == VK_SUCCESS);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    write.dstBinding = 0;
    write.dstSet = set;
    write.pTexelBufferView = &bufferView;
    vkUpdateDescriptorSets(GetDevice(), 1, &write, 0, nullptr);

    auto bridge = registry->Get<IBridge>();

    bridge->Register(ShaderSourceMappingMessage::kID, registry->AddNew<ShaderSGUIDHostListener>());

    auto listener = registry->New<WritingNegativeValueListener>(registry);
    bridge->Register(WritingNegativeValueMessage::kID, listener);

    MessageStream stream;
    {
        MessageStreamView view(stream);

        // Make the recording wait for compilation
        auto config = view.Add<SetInstrumentationConfigMessage>();
        config->synchronousRecording = 1;

        // Global instrumentation
        auto msg = view.Add<SetGlobalInstrumentationMessage>();
        msg->featureBitSet = ~0ull;
    }

    bridge->GetOutput()->AddStream(stream);
    bridge->Commit();

    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        REQUIRE(vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &set, 0, nullptr);

        vkCmdDispatch(commandBuffer, 4, 1, 1);

        vkEndCommandBuffer(commandBuffer);

        // Submit the command buffer
        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.pCommandBuffers = &commandBuffer;
        submit.commandBufferCount = 1;
        REQUIRE(vkQueueSubmit(GetPrimaryQueue(), 1, &submit, VK_NULL_HANDLE) == VK_SUCCESS);
    }

    // Wait for the results
    vkQueueWaitIdle(GetPrimaryQueue());

    // Listener must have been invoked
    REQUIRE(listener->visited);

    // Release handles
    vkDestroyPipeline(GetDevice(), pipeline, nullptr);
    vkDestroyShaderModule(GetDevice(), shaderModule, nullptr);
    vkDestroyPipelineLayout(GetDevice(), pipelineLayout, nullptr);
    vkFreeCommandBuffers(GetDevice(), commandPool, 1, &commandBuffer);
    vkDestroyCommandPool(GetDevice(), commandPool, nullptr);
}

