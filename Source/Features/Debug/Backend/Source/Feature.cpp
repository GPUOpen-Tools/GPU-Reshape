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

// Feature
#include <Features/Debug/Feature.h>
#include <Features/Debug/WatchpointHeader.h>
#include <Features/Debug/LooseAcquisitionProgram.h>
#include <Features/Debug/ResetHeaderProgram.h>
#include <Features/Debug/FinalizePredicateProgram.h>
#include <Features/Debug/IndirectCopyProgram.h>
#include <Features/Debug/SetupIndirectCopyProgram.h>
#include <Features/Debug/SetupPredicateProgram.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/Emitters/ResourceTokenEmitter.h>
#include <Backend/IL/InstructionValueCommon.h>
#include <Backend/CommandContext.h>
#include <Backend/SubmissionContext.h>
#include <Backend/Command/CommandBuilder.h>
#include <Backend/IL/Emitters/ExtendedEmitter.h>
#include <Backend/IL/Metadata/KernelMetadata.h>
#include <Backend/Scheduler/IScheduler.h>
#include <Backend/Scheduler/SchedulerTileMapping.h>
#include <Backend/IL/ShaderStruct.h>
#include <Backend/IL/ShaderBufferStruct.h>
#include <Backend/ShaderProgram/IShaderProgramHost.h>
#include <Backend/Device/DeviceState.h>
#include <Backend/Device/DeviceStateRef.h>
#include <Backend/IL/Tiny/TinyType.h>
#include <Backend/IL/Tiny/TinyTypePacking.h>
#include <Backend/IL/TypeSize.h>
#include <Backend/IL/Emitters/IDebugEmitter.h>
#include <Backend/Device/IDeviceProperties.h>
#include <Backend/IL/Analysis/CFG/DominatorAnalysis.h>

// Generated schema
#include <Schemas/Features/Debug.h>
#include <Schemas/Features/DebugConfig.h>
#include <Schemas/Log.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStreamCommon.h>

// Bridge
#include <Bridge/Log/LogBuffer.h>
#include <Bridge/IBridge.h>

// Common
#include <Common/FileSystem.h>
#include <Common/Registry.h>
#include <Common/Hash.h>

/// Use predication for host streaming copies?
#define USE_PREDICATION 1

/// Use tiled resources for allocation?
#define USE_TILED 1

/// Max number of watchpoints, TODO[dbg]: for now?
static constexpr uint32_t kMaxWatchpoints = 1 << 16;

/// Maximum streaming size
#if USE_TILED
static constexpr uint64_t kDebugStreamBufferSize = UINT32_MAX; // ~4gb
#else // USE_TILED
static constexpr uint64_t kDebugStreamBufferSize = UINT32_MAX / 4;
#endif // USE_TILED

bool DebugFeature::Install() {
    // Must have the export host
    auto exportHost = registry->Get<IShaderExportHost>();
    if (!exportHost) {
        return false;
    }

    // Shader data host
    shaderDataHost = registry->Get<IShaderDataHost>();

    // Allocate the shared export
    auto messageType = ShaderExportTypeInfo::FromType<WatchpointAcquisitionMessage>();
    messageType.streamType = ShaderExportStreamType::Input;
    exportID = exportHost->Allocate(messageType);

    // Optional sguid host
    sguidHost = registry->Get<IShaderSGUIDHost>();

    // Get scheduler
    scheduler = registry->Get<IScheduler>();
    
    // Get device
    deviceProperties = registry->Get<IDeviceProperties>();
    if (!deviceProperties) {
        return false;
    }
    
    // Get capabilities
    deviceCapabilityTable = deviceProperties->GetCapabilityTable();
#if !USE_PREDICATION
    deviceCapabilityTable.supportsPredicates = false;
#endif // !USE_PREDICATION

    // Create monotonic primitive
    exclusiveTransferPrimitiveID = scheduler->CreatePrimitive();

    // Create lifetime queue
    contextLifetimeQueue.Install(scheduler);

    // Create tiled streaming buffer
    streamBufferID = shaderDataHost->CreateBuffer(ShaderDataBufferInfo {
        .elementCount = kDebugStreamBufferSize / sizeof(uint32_t),
        .format = Backend::IL::Format::R32UInt,
#if USE_TILED
        .flagSet = ShaderDataBufferFlag::Tiled | ShaderDataBufferFlag::Predicate
#else // USE_TILED
        .flagSet = ShaderDataBufferFlag::Predicate
#endif // USE_TILED
    }, "DebugStreamBuffer");

    // Create allocator for the streaming buffer
    buddyAllocator.Install(kDebugStreamBufferSize + 1);

    // Create residency handler for the streaming buffer
    tileResidencyAllocator.Install(kDebugStreamBufferSize);
    
    // Try to create programs
    if (!CreatePrograms()) {
        return false;
    }

    // Register for messages
    bridge = registry->Get<IBridge>().GetUnsafe();
    bridge->Register(WatchpointAcquisitionMessage::kID, this);

    // Get state voter, used primarily for scheduler changes
    stateVote = registry->Get<IDeviceStateVote>();

    // Get the debug emitter
    debugEmitter = registry->Get<IL::IDebugEmitter>();

    // Allocate watchpoint headers
    buddyAllocator.Allocate(kMaxWatchpoints * sizeof(WatchpointHeader));
    tileResidencyAllocator.Allocate(0, kMaxWatchpoints * sizeof(WatchpointHeader));
    
    // OK
    return true;
}

bool DebugFeature::CreatePrograms() {
    // Must have program host
    auto programHost = registry->Get<IShaderProgramHost>();
    if (!programHost) {
        return false;
    }
    
    // Create predicated programs if possible
    if (deviceCapabilityTable.supportsPredicates) {
        setupPredicateProgram = registry->New<SetupPredicateProgram>(streamBufferID, exportID);
        if (!setupPredicateProgram->Install()) {
            return false;
        }
    
        finalizePredicateProgram = registry->New<FinalizePredicateProgram>(streamBufferID, exportID);
        if (!finalizePredicateProgram->Install()) {
            return false;
        }
    } else {
        setupIndirectCopyProgram = registry->New<SetupIndirectCopyProgram>(streamBufferID, exportID);
        if (!setupIndirectCopyProgram->Install()) {
            return false;
        }
    
        indirectCopyProgram = registry->New<IndirectCopyProgram>(streamBufferID, exportID);
        if (!indirectCopyProgram->Install()) {
            return false;
        }
    }

    // Create the acq. program
    looseAcquisitionProgram = registry->New<LooseAcquisitionProgram>(streamBufferID, exportID);
    if (!looseAcquisitionProgram->Install()) {
        return false;
    }

    // Create the reset program
    resetHeaderProgram = registry->New<ResetHeaderProgram>(streamBufferID);
    if (!resetHeaderProgram->Install()) {
        return false;
    }

    // Register programs
    looseAcquisitionProgramID = programHost->Register(looseAcquisitionProgram);
    resetHeaderProgramID = programHost->Register(resetHeaderProgram);
    return true;
}

FeatureHookTable DebugFeature::GetHookTable() {
    FeatureHookTable table{};
    table.preSubmit = BindDelegate(this, DebugFeature::OnPreSubmit);
    table.syncPoint = BindDelegate(this, DebugFeature::OnSyncPoint);
    table.join = BindDelegate(this, DebugFeature::OnJoin);
    return table;
}

void DebugFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void DebugFeature::CollectMessages(IMessageStorage *storage) {
    
}

void DebugFeature::Inject(IL::Program &program, const MessageStreamView<> &specialization) {
    std::unordered_map<uint32_t, DebugWatchpointMessage> watchpointStreams;

    // Options
    if (const DebugConfigMessage* debugConfig = Find<DebugConfigMessage>(specialization)) {
        // Parse all watchpoints
        for (auto it = ConstMessageStreamView<DebugWatchpointMessage, MessageSubStream>(debugConfig->watchpoints).GetIterator(); it; ++it) {
            watchpointStreams[it->codeOffset] = *it.Get();
        }
    }

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        // No reason to instrument terminators
        // Also conveniently hide issue with pointing past terminators
        if (it == context.basicBlock.GetTerminator()) {
            return it;
        }
        
        if (auto watchpointIt = watchpointStreams.find(it->source.codeOffset); watchpointIt != watchpointStreams.end()) {
            return InjectWatchpoint(context, it, watchpointIt->second);
        }

        // TODO[dbg]: Let's not reiterate the everything 
        return it;
    });
}

void DebugFeature::Handle(const MessageStream *streams, uint32_t count) {
    std::lock_guard guard(mutex);
    
    // Command buffer for stages
    CommandBuffer  buffer;
    CommandBuilder builder(buffer);
    
    for (uint32_t i = 0; i < count; i++) {
        // Handle GPU feedback
        // TODO[dbg]: This is ugly
        if (streams[i].GetSchema().type == MessageSchemaType::Chunked) {
            for (auto it = ConstMessageStreamView<WatchpointAcquisitionMessage>(streams[i]).GetIterator(); it; ++it) {
                OnWatchpointAcquired(it.Get(), builder);
            }
            continue;
        }
        
        ConstMessageStreamView view(streams[i]);

        // Visit all ordered messages
        for (ConstMessageStreamView<>::ConstIterator it = view.GetIterator(); it; ++it) {
            switch (it.GetID()) {
                case DebugWatchpointStreamHandledMessage::kID: {
                    const DebugWatchpointStreamHandledMessage *msg = it.Get<DebugWatchpointStreamHandledMessage>();
                    defaultController.remoteRequestIndex = msg->request;
                    break;
                }
                case RegisterDebugWatchpointMessage::kID: {
                    const RegisterDebugWatchpointMessage *msg = it.Get<RegisterDebugWatchpointMessage>();

                    // Add watchpoint
                    Watchpoint& watchpoint = watchpoints.emplace_back();
                    watchpoint.uid = msg->uid;
                    watchpoint.captureMode = static_cast<WatchpointCaptureMode>(msg->captureMode);
                    watchpoint.streamSize = msg->streamSize;

                    // Setup payload
                    CreateAndUpdatePayload(watchpoint);

                    // Assign watchpoint device states
                    if (!poolingState.IsSet()) {
                        // Greatly increase pooling rate, speeds up watchpoint streaming
                        poolingState = DeviceStateRef(stateVote.GetUnsafe(), DeviceStatePooling {
                            .intervalMS = 1
                        });
                    }
                    
                    break;
                }
                case ReallocateDebugWatchpointMessage::kID: {
                    const ReallocateDebugWatchpointMessage *msg = it.Get<ReallocateDebugWatchpointMessage>();

                    // Try to find it
                    Watchpoint* watchpoint = FindWatchpointNoLock(msg->uid);
                    if (!watchpoint) {
                        break;
                    }

                    // Push the old allocation to the queue
                    allocationDestructionQueue.push_back(PendingDestruction {
                        .allocation = watchpoint->streamAllocation,
                        .hostStreamingBuffer = watchpoint->hostStreamingBuffer,
                        .lastCommit = contextLifetimeQueue.GetCommitHead()
                    });

                    // Set new streaming size
                    watchpoint->streamSize = msg->streamSize;

                    // Setup payload
                    CreateAndUpdatePayload(*watchpoint);

                    // Assign watchpoint device states
                    if (!poolingState.IsSet()) {
                        // Greatly increase pooling rate, speeds up watchpoint streaming
                        poolingState = DeviceStateRef(stateVote.GetUnsafe(), DeviceStatePooling {
                            .intervalMS = 1
                        });
                    }
                    
                    break;
                }
                case DeregisterDebugWatchpointMessage::kID: {
                    const DeregisterDebugWatchpointMessage *msg = it.Get<DeregisterDebugWatchpointMessage>();

                    // TODO: This isn't really correct, since we're also not waiting for the instrumentation to commit the old stuff, and the pending submissions...
                    // Tricky area to get right. We could have an "invalidated" header region, since the memory is technically still valid for a bit, but not sure.

                    // Find watchpoint
                    auto watchpoint = std::ranges::find_if(watchpoints, [&](const Watchpoint& candidate) {
                        return candidate.uid == msg->uid;
                    });

                    // Shouldn't happen, but just in case
                    if (watchpoint == watchpoints.end()) {
                        break;
                    }

                    // Free its memory
                    allocationDestructionQueue.push_back(PendingDestruction {
                        .allocation = watchpoint->streamAllocation,
                        .hostStreamingBuffer = watchpoint->hostStreamingBuffer,
                        .lastCommit = contextLifetimeQueue.GetCommitHead()
                    });

                    // No longer tracked
                    watchpoints.erase(watchpoint);

                    // Reset watchpoint device states
                    if (watchpoints.empty()) {
                        poolingState = {};
                    }
                    break;
                }
            }
        }
    }
}

void DebugFeature::OnPreSubmit(SubmissionContext &submitContext, const CommandContextHandle *contexts, uint32_t contextCount) {
    std::lock_guard guard(mutex);

    // Anything to sync?
    bool hasSyncRequest = false;

    // Any tiles pending mapping?
#if USE_TILED
    if (tileResidencyAllocator.GetRequestCount()) {
        hasSyncRequest = true;
        
        // All mappings
        std::vector<SchedulerTileMapping> tileMappings;
        tileMappings.reserve(tileResidencyAllocator.GetRequestCount());

        // Map all new requests
        for (uint32_t i = 0; i < tileResidencyAllocator.GetRequestCount(); i++) {
            const TileMappingRequest& request = tileResidencyAllocator.GetRequest(i);

            // Create mapping and push for mapping
            tileMappings.push_back(SchedulerTileMapping {
                .mapping = shaderDataHost->CreateMapping(streamBufferID, request.tileCount),
                .tileOffset = request.tileOffset,
                .tileCount = request.tileCount
            });
        }

        // Cleanup
        tileResidencyAllocator.ClearRequests();

        // Create the tile mappings for the new resource
        scheduler->MapTiles(Queue::ExclusiveTransfer, streamBufferID, static_cast<uint32_t>(tileMappings.size()), tileMappings.data());
    }
#endif // USE_TILED

    // No watchpoints, let's not do redundant work
    if (watchpoints.empty() && !hasSyncRequest) {
        return;
    }

    // Sync buffer
    CommandBuffer syncBuffer;
    CommandBuilder syncBuilder(syncBuffer);

    // Handle header mappings
    for (Watchpoint& watchpoint : watchpoints) {
        if (!watchpoint.pendingTransferHeader) {
            continue;
        }

        // Reset the header
        syncBuilder.StageBuffer(streamBufferID, watchpoint.uid * sizeof(WatchpointHeader), sizeof(WatchpointHeader), &watchpoint.header);
        watchpoint.pendingTransferHeader = false;
        
        // Always sync header resets
        hasSyncRequest = true;
    }

    if (hasSyncRequest) {
        // Allocate the next sync value
        ++exclusiveTransferPrimitiveMonotonicCounter;
        
        // Submit to the transfer queue
        SchedulerPrimitiveEvent event;
        event.id = exclusiveTransferPrimitiveID;
        event.value = exclusiveTransferPrimitiveMonotonicCounter;
        scheduler->Schedule(Queue::ExclusiveTransfer, syncBuffer, &event);
    }

    // Submissions always wait for the last mappings
    submitContext.waitPrimitives.Add(SchedulerPrimitiveEvent {
        .id = exclusiveTransferPrimitiveID,
        .value = exclusiveTransferPrimitiveMonotonicCounter
    });
    
    // Pre-context, used for atomically resetting headers
    CommandBuilder preBuilder(submitContext.preContext->buffer);
    {
        // Reset all pending headers
        for (Watchpoint& watchpoint: watchpoints) {
            if (!watchpoint.pendingHeaderReset) {
                continue;
            }
           
            // Setup data
            WatchpointResetHeaderData data;
            data.allocationDWordOffset = watchpoint.uid * WatchpointHeaderDWordCount;

            // Dispatch loose update
            preBuilder.SetShaderProgram(resetHeaderProgramID);
            preBuilder.SetDescriptorData(resetHeaderProgram->GetDataID(), data);
            preBuilder.Dispatch(1, 1, 1);
            preBuilder.UAVBarrier();
            watchpoint.pendingHeaderReset = false;
        }
    }
    
    // Post-context, used for post signalling and copies
    CommandBuilder postBuilder(submitContext.postContext->buffer);
    {
        // Wait for any ongoing shaders
        postBuilder.UAVBarrier();
        postBuilder.SetShaderProgram(looseAcquisitionProgramID);

        // Loose updates require gpu visibility
        for (Watchpoint& watchpoint: watchpoints) {
            if (watchpoint.captureMode != WatchpointCaptureMode::AllEvents) {
                continue;
            }
           
            // Setup data
            WatchpointLooseAcquisitionData patchData;
            patchData.allocationDWordOffset = watchpoint.uid * WatchpointHeaderDWordCount;
            patchData.watchpointUid = watchpoint.uid;

            // Dispatch loose update
            postBuilder.SetDescriptorData(looseAcquisitionProgram->GetDataID(), patchData);
            postBuilder.Dispatch(1, 1, 1);
        }

        // Wait for updates
        postBuilder.UAVBarrier();

        // Try to use predicated copies, if possible
        if (deviceCapabilityTable.supportsPredicates) {
            // Setup all predication commands
            postBuilder.SetShaderProgram(setupPredicateProgram->GetID());
            for (const Watchpoint& watchpoint : watchpoints) {
                WatchpointCopyData data;
                data.allocationDWordOffset = watchpoint.uid * WatchpointHeaderDWordCount;
                postBuilder.SetDescriptorData(setupPredicateProgram->GetDataID(), data);
                postBuilder.Dispatch(1, 1, 1);
            }
            
            // Wait for setup
            postBuilder.UAVBarrier();

            // Copy the debug streaming buffer to host
            for (const Watchpoint& watchpoint : watchpoints) {
                // If not supported, pay for the expensive copy
                if (deviceCapabilityTable.supportsPredicates) {
                    postBuilder.BeginPredicate(
                        streamBufferID,
                        watchpoint.uid * sizeof(WatchpointHeader) + offsetof(WatchpointHeader, predicationLo)
                    );
                }
                
                // TODO[dbg]: Now we're doing two copies, not so nice
                postBuilder.CopyBuffer(
                    streamBufferID, watchpoint.uid * sizeof(WatchpointHeader),
                    watchpoint.hostStreamingBuffer, 0,
                    sizeof(WatchpointHeader)
                );
                
                postBuilder.CopyBuffer(
                    streamBufferID, watchpoint.streamAllocation.offset,
                    watchpoint.hostStreamingBuffer, sizeof(WatchpointHeader),
                    watchpoint.streamAllocation.length
                );
                
                // If not supported, pay for the expensive copy
                if (deviceCapabilityTable.supportsPredicates) {
                    postBuilder.EndPredicate(streamBufferID);
                }
            }
            
            // Finalize all predicate states
            postBuilder.SetShaderProgram(finalizePredicateProgram->GetID());
            for (const Watchpoint& watchpoint : watchpoints) {
                WatchpointCopyData data;
                data.allocationDWordOffset = watchpoint.uid * WatchpointHeaderDWordCount;
                postBuilder.SetDescriptorData(finalizePredicateProgram->GetDataID(), data);
                postBuilder.Dispatch(1, 1, 1);
            }
        } else {
            // Setup indirect commands
            postBuilder.SetShaderProgram(setupIndirectCopyProgram->GetID());
            for (const Watchpoint& watchpoint : watchpoints) {
                WatchpointCopyData data;
                data.allocationDWordOffset = watchpoint.uid * WatchpointHeaderDWordCount;
                postBuilder.SetDescriptorData(setupIndirectCopyProgram->GetDataID(), data);
                postBuilder.SetResourceData(setupIndirectCopyProgram->GetHostDataBinding(), watchpoint.hostStreamingBuffer);
                postBuilder.Dispatch(1, 1, 1);
            }

            // Execute indirect commands
            postBuilder.SetShaderProgram(indirectCopyProgram->GetID());
            for (const Watchpoint& watchpoint : watchpoints) {
                WatchpointCopyData data;
                data.allocationDWordOffset = watchpoint.uid * WatchpointHeaderDWordCount;
                postBuilder.SetDescriptorData(indirectCopyProgram->GetDataID(), data);
                postBuilder.SetResourceData(indirectCopyProgram->GetHostDataBinding(), watchpoint.hostStreamingBuffer);
                postBuilder.DispatchIndirect(
                    streamBufferID,
                    watchpoint.uid * sizeof(WatchpointHeader) + offsetof(WatchpointHeader, copyDispatchParams)
                );
            }
        
            postBuilder.UAVBarrier();
        }
    }

    // Add to the tracker
    contextLifetimeQueue.Enqueue(submitContext, contexts, contextCount);
}

void DebugFeature::OnJoin(CommandContextHandle contextHandle) {
    std::lock_guard guard(mutex);
    contextLifetimeQueue.Join(contextHandle);
}

bool DebugFeature::ThrottleController(RequestController &controller) {
    TimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now());

    // Passed scheduled interval?
    if (now - controller.lastStreamBufferTime < Duration(static_cast<uint32_t>(controller.streamBufferInterval))) {
        return false;
    }

    // Determine the number of in-flight requests
    uint32_t window = controller.requestIndex - controller.remoteRequestIndex;

    // TODO[dbg]: This "works", but the magic constants aren't that good
    if (window > kRequestThrottleWindow) {
        controller.streamBufferInterval *= 1.2;
    } else {
        controller.streamBufferInterval *= 0.8;
    }

    // Set new streaming interval, clamped to bounds
    controller.lastStreamBufferTime = now;
    controller.streamBufferInterval = std::clamp(controller.streamBufferInterval, kMinDurationBeforeThrottle, kMaxDurationBeforeDrop);

    // OK
    return true;
}

bool DebugFeature::CanCollectWatchpoint(const Watchpoint &watchpoint) {
    switch (watchpoint.captureMode) {
        default:
            ASSERT(false, "Invalid capture mode");
            return false;
        case WatchpointCaptureMode::FirstEvent:
        case WatchpointCaptureMode::FirstViewport:
        case WatchpointCaptureMode::AllEvents:
            return watchpoint.pendingCollection;
    }
}

uint32_t DebugFeature::GetWatchpointInstrumentationHash(const Watchpoint &watchpoint, const WatchpointHeader *header) {
    switch (watchpoint.captureMode) {
        default:
            ASSERT(false, "Invalid capture mode");
            return 0u;
        case WatchpointCaptureMode::FirstEvent:
        case WatchpointCaptureMode::FirstViewport:
        case WatchpointCaptureMode::AllEvents:
            return watchpoint.pendingCollectionHash;
    }
}

bool DebugFeature::HasWatchpointStreambackData(const Watchpoint& watchpoint, const WatchpointHeader* header) {
    switch (watchpoint.captureMode) {
        default:
            ASSERT(false, "Invalid capture mode");
            return false;
        case WatchpointCaptureMode::FirstEvent:
        case WatchpointCaptureMode::FirstViewport:
            return true;
        case WatchpointCaptureMode::AllEvents:
            return watchpoint.pendingAcqDynamicCounter > 0;
    }
}

uint64_t DebugFeature::GetWatchpointStreamRequestSize(const Watchpoint &watchpoint, const WatchpointHeader *header, const WatchpointDataHostLayout& hostLayout) {
    switch (watchpoint.captureMode) {
        default:
            ASSERT(false, "Invalid capture mode");
            return 0;
        case WatchpointCaptureMode::FirstEvent:
        case WatchpointCaptureMode::FirstViewport:
            // Just stream back the entire thing
            return header->dwordStreamCount * sizeof(uint32_t);
        case WatchpointCaptureMode::AllEvents:
            // Stream back the actual contents
            return watchpoint.pendingAcqDynamicCounter * (sizeof(WatchpointLooseHeader) + hostLayout.dataDWordStride * sizeof(uint32_t));
    }
}

void DebugFeature::OnSyncPoint() {
    std::lock_guard guard(mutex);

    // No watchpoints? No data
    if (watchpoints.empty()) {
        return;
    }

    // Remove dead allocations
    allocationDestructionQueue.erase(std::ranges::remove_if(allocationDestructionQueue, [&](const PendingDestruction& pending) {
        if (!contextLifetimeQueue.IsCommitted(pending.lastCommit)) {
            return false;
        }

        // No longer in use, free the memory
        buddyAllocator.Free(pending.allocation);

        // TODO[dbg]: Fix the bug!
        // shaderDataHost->Destroy(pending.hostStreamingBuffer);
        return true;
    }).begin(), allocationDestructionQueue.end());

    // Throttle the data requests
    if (!ThrottleController(defaultController)) {
        return;
    }

    // Buffer for stages
    CommandBuffer buffer;
    CommandBuilder builder(buffer);
    
    // Stream out the watchpoints separately
    for (Watchpoint& watchpoint : watchpoints) {
        // Pending collection?
        if (!CanCollectWatchpoint(watchpoint)) {
            continue;
        }
        
        // Map the streaming buffer
        void* mapped = shaderDataHost->Map(watchpoint.hostStreamingBuffer);

        // Payload is after the header
        auto* header  = static_cast<WatchpointHeader*>(mapped);
        void* payload = header + 1;

        // Do we have any data at all?
        if (HasWatchpointStreambackData(watchpoint, header)) {
            // Get the instrumentation hash, this makes sure the host layout is always in sync
            uint32_t instrumentationHash32 = GetWatchpointInstrumentationHash(watchpoint, header);

            // Make sure it's a valid layout
            auto hostLayoutIt = watchpoint.hostLayoutMap.find(instrumentationHash32);
            if (hostLayoutIt == watchpoint.hostLayoutMap.end()) {
                continue;
            }

            // Describes the expected memory layout
            WatchpointDataHostLayout& hostLayout = hostLayoutIt->second;
            
            // How much we actually need to stream
            uint64_t requestedStreamSize = GetWatchpointStreamRequestSize(watchpoint, header, hostLayout);
            uint64_t effectiveStreamSize = std::min(watchpoint.streamSize, requestedStreamSize);
        
            // Empty out last stream
            MessageStreamView<DebugWatchpointStreamMessage> view(stream);

            // Allocate watchpoint data
            auto message = view.Add(DebugWatchpointStreamMessage::AllocationInfo {
                .dataCount = effectiveStreamSize,
                .dataTinyTypeCount = hostLayout.tinyType.size()
            });

            // TODO[dbg]: Can we somehow map this in-place? There's a lot of copies going on
            std::memcpy(message->data.Get(), payload, effectiveStreamSize);

            // Copy over tiny type
            std::memcpy(message->dataTinyType.Get(), hostLayout.tinyType.data(), hostLayout.tinyType.size());

            // Write out request data
            message->request = ++defaultController.requestIndex;
            message->uid = watchpoint.uid;
            message->captureMode = static_cast<uint32_t>(watchpoint.captureMode);
            message->dataFormat = static_cast<uint32_t>(hostLayout.format);
            message->dataTypeId = hostLayout.typeId;
            message->dataCompression = static_cast<uint32_t>(hostLayout.compression);
            message->dataDWordStride = hostLayout.dataDWordStride;
            message->dataOrder = static_cast<uint32_t>(header->dataOrder);
            message->dataStaticWidth = header->staticWidth;
            message->dataStaticHeight = header->staticHeight;
            message->dataStaticDepth = header->staticDepth;
            message->dataDynamicCounter = watchpoint.pendingAcqDynamicCounter;
            message->dataRequestStreamSize = static_cast<uint32_t>(requestedStreamSize);
        }

        // Done!
        shaderDataHost->Unmap(watchpoint.hostStreamingBuffer, mapped);

        // Clear the buffer, if needed
        if (watchpoint.captureMode == WatchpointCaptureMode::FirstEvent || watchpoint.captureMode == WatchpointCaptureMode::FirstViewport) {
            builder.ClearBuffer(
                streamBufferID,
                watchpoint.streamAllocation.offset,
                watchpoint.streamAllocation.length,
                0x0
            );
        }

        // Patch the header on next submission
        // TODO: We could submit it separately if too slow
        watchpoint.pendingHeaderReset = true;

        // Collected!
        watchpoint.pendingCollection = false;
    }

    // Any commands?
    if (buffer.Count()) {
        // Allocate the next sync value
        ++exclusiveTransferPrimitiveMonotonicCounter;
        
        // Submit to the transfer queue
        SchedulerPrimitiveEvent event;
        event.id = exclusiveTransferPrimitiveID;
        event.value = exclusiveTransferPrimitiveMonotonicCounter;
        scheduler->Schedule(Queue::ExclusiveTransfer, buffer, &event);
    }
    
    // Any immediate streams?
    if (!stream.IsEmpty()) {
        bridge->GetOutput()->AddStreamAndSwap(stream);
    }
}

void DebugFeature::OnWatchpointAcquired(const WatchpointAcquisitionMessage *acqMessage, CommandBuilder& builder) {
    ASSERT(acqMessage->magic == 42, "Corrupt message");
    
    // If it failed to resolve, it may have been removed
    if (Watchpoint *watchpoint = FindWatchpointNoLock(acqMessage->uid)) {
        if (watchpoint->captureMode == WatchpointCaptureMode::AllEvents) {
            uint32_t instrumentationHash32 = *(reinterpret_cast<const uint32_t*>(acqMessage) + 1);
            
            // TODO: We could move this to a double-exchange on the GPU
            if (instrumentationHash32 == kWatchpointInstrumentationHashLocked) {
                return;
            }
            
            // Loose events may double-signal
            watchpoint->pendingCollection = true;
        
            // Read beyond primary key
            // TODO[init]: Add support for reading chunks in C++
            watchpoint->pendingCollectionHash = instrumentationHash32;
            watchpoint->pendingAcqDynamicCounter = *(reinterpret_cast<const uint32_t*>(acqMessage) + 2);
        } else {
            // Should be an assertion, but let's have this be recoverable
            if (watchpoint->pendingCollection) {
                registry->Get<LogBuffer>()->Add(
                    "Debug",
                    LogSeverity::Error,
                    "Watchpoint double acquisition, synchronization error"
                );
            }
            
            watchpoint->pendingCollection = true;
        
            // Read beyond primary key
            // TODO[init]: Add support for reading chunks in C++
            watchpoint->pendingCollectionHash = *(reinterpret_cast<const uint32_t*>(acqMessage) + 1);
        }
    }
}

IL::BasicBlock::Iterator DebugFeature::SplitInterruptBlock(const IL::VisitContext& context, IL::BasicBlock::Iterator it, IL::BasicBlock *interruptBlock) {
    // Split the iterator to resume
    // Excluding the iterator itself, since we may want to reference the instruction results
    IL::BasicBlock* resumeBlock = context.function.GetBasicBlocks().AllocBlock();
    it.block->Split(resumeBlock, std::next(it));

    // Branch the split-end to interrupt
    IL::Emitter(context.program, *it.block).Branch(interruptBlock);
    
    // Branch the resume to interrupt
    IL::Emitter(context.program, *interruptBlock).Branch(resumeBlock);
    return resumeBlock->begin();
}

static bool IsTypeSerializationSupported(const Backend::IL::Type* type) {
    switch (type->kind) {
        default: {
            // TODO[dbg]: Matrix, Array
            // TODO[dbg]: Resources PRMT!
            return false;
        }
        case Backend::IL::TypeKind::Bool:
        case Backend::IL::TypeKind::Int:
        case Backend::IL::TypeKind::FP: {
            // Always supported
            return true;
        }
        case Backend::IL::TypeKind::Vector: {
            auto typed = type->As<Backend::IL::VectorType>();

            // Supports serialization if the contained can be
            return IsTypeSerializationSupported(typed->containedType);
        }
        case Backend::IL::TypeKind::Struct: {
            auto typed = type->As<Backend::IL::StructType>();

            // Supports serialization if every member can be
            for (const Backend::IL::Type *member: typed->memberTypes) {
                if (!IsTypeSerializationSupported(member)) {
                    return false;
                }
            }

            return true;
        }
    }
}

static bool SupportsImageFPUnormCompression(const IL::Instruction* instr, const Backend::IL::Type* type) {
    switch (type->kind) {
        default: {
            // Nope
            return false;
        }
        case Backend::IL::TypeKind::FP: {
            // Always supported
            return true;
        }
        case Backend::IL::TypeKind::Vector: {
            auto typed = type->As<Backend::IL::VectorType>();

            // Supports compression if the contained can be
            // Up to 4 components
            return SupportsImageFPUnormCompression(instr, typed->containedType) && typed->dimension <= 4u;
        }
        case Backend::IL::TypeKind::Struct: {
            auto typed = type->As<Backend::IL::StructType>();

            // Only supported for special case loads/samples
            if (!instr->Is<IL::LoadBufferInstruction>() &&
                !instr->Is<IL::LoadTextureInstruction>() &&
                !instr->Is<IL::SampleTextureInstruction>()) {
                return false;
            }

            // Supports compression if every member can be
            for (const Backend::IL::Type *member: typed->memberTypes) {
                if (!SupportsImageFPUnormCompression(instr, member)) {
                    return false;
                }
            }

            return true;
        }
    }
}

static IL::ID CompressFPUnorm8888(const IL::VisitContext &context, IL::Emitter<> emitter, IL::ID value) {
    const Backend::IL::Type *type = context.program.GetTypeMap().GetType(value);

    // Extended emitter for common operations
    IL::ExtendedEmitter extended(emitter);

    // Handle type
    switch (type->kind) {
        default: {
            ASSERT(false, "Invalid");
            return IL::InvalidID;
        }
        case Backend::IL::TypeKind::Vector: {
            auto* typed = type->As<Backend::IL::VectorType>();

            // Compose color byte by byte
            IL::ID pixel = emitter.UInt32(0);
            for (uint32_t i = 0; i < typed->dimension; i++) {
                IL::ID component = CompressFPUnorm8888(context, emitter, emitter.Extract(value, emitter.UInt32(i)));
                pixel = emitter.BitOr(pixel, emitter.BitShiftLeft(component, emitter.UInt32(i * 8)));
            }
            return pixel;
        }
        case Backend::IL::TypeKind::Struct: {
            auto* typed = type->As<Backend::IL::StructType>();

            // Compose color byte by byte
            IL::ID pixel = emitter.UInt32(0);
            for (uint32_t i = 0; i < typed->memberTypes.size(); i++) {
                IL::ID component = CompressFPUnorm8888(context, emitter, emitter.Extract(value, emitter.UInt32(i)));
                pixel = emitter.BitOr(pixel, emitter.BitShiftLeft(component, emitter.UInt32(i * 8)));
            }
            return pixel;
        }
        case Backend::IL::TypeKind::FP: {
            // TODO[dbg]: Color space handling
            IL::ID linear = emitter.Mul(extended.Pow(value, context.program.GetConstants().FP(0.4545454545f)->id), context.program.GetConstants().FP(255.0f)->id);
            IL::ID quant = emitter.FloatToUInt32(linear); // This is my quant
            return extended.Min(quant, emitter.UInt32(255));
        }
        case Backend::IL::TypeKind::Int:  {
            value = emitter.BitCast(value, context.program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{ .bitWidth = 32 }));
            return extended.Min(value, emitter.UInt32(255));
        }
        case Backend::IL::TypeKind::Bool: {
            return emitter.Select(value, emitter.UInt32(255), emitter.UInt32(0));
        }
    }
}

static void FillValueStream(MessageStreamView<DebugWatchpointValueMetadataMessage> view, const IL::DebugSingleValue& value, uint32_t& id) {
    DebugWatchpointValueMetadataMessage *data = view.Add(DebugWatchpointValueMetadataMessage::AllocationInfo {
        .nameLength = value.name.length()
    });
    
    data->name.Set(value.name);
    data->valueId = id++;
    data->hasReconstruction = false;
    
    switch (value.type->kind) {
        default: {
            data->hasReconstruction = value.handle != nullptr;
            break;
        }
        case Backend::IL::TypeKind::Struct: {
            auto _type = value.type->As<Backend::IL::StructType>();
            
            for (uint32_t i = 0; i < _type->memberTypes.size(); i++) {
                FillValueStream(view, value.values[i], id);
            }
            
            break;
        }
        case Backend::IL::TypeKind::Array: {
            auto _type = value.type->As<Backend::IL::ArrayType>();
            
            for (uint32_t i = 0; i < _type->count; i++) {
                FillValueStream(view, value.values[i], id);
            }

            break;
        }
        case Backend::IL::TypeKind::Vector: {
            auto _type = value.type->As<Backend::IL::VectorType>();
            
            for (uint32_t i = 0; i < _type->dimension; i++) {
                FillValueStream(view, value.values[i], id);
            }

            break;
        }
        case Backend::IL::TypeKind::Matrix: {
            auto _type = value.type->As<Backend::IL::MatrixType>();
            
            for (uint32_t column = 0; column < _type->columns; column++) {
                for (uint32_t row = 0; row < _type->rows; row++) {
                    FillValueStream(view, value.values[column * _type->rows + row], id);
                }
            }

            break;
        }
    }
}

static const IL::DebugSingleValue* GetValueFromId(const IL::DebugSingleValue& value, uint32_t& id) {
    if (!id) {
        return &value;
    }
    
    switch (value.type->kind) {
        default: {
            return nullptr;
        }
        case Backend::IL::TypeKind::Struct: {
            auto _type = value.type->As<Backend::IL::StructType>();
            
            for (uint32_t i = 0; i < _type->memberTypes.size(); i++) {
                if (const IL::DebugSingleValue *member = GetValueFromId(value.values[i], --id)) {
                    return member;
                }
            }
            
            return nullptr;
        }
        case Backend::IL::TypeKind::Array: {
            auto _type = value.type->As<Backend::IL::ArrayType>();
            
            for (uint32_t i = 0; i < _type->count; i++) {
                if (const IL::DebugSingleValue *member = GetValueFromId(value.values[i], --id)) {
                    return member;
                }
            }

            return nullptr;
        }
        case Backend::IL::TypeKind::Vector: {
            auto _type = value.type->As<Backend::IL::VectorType>();
            
            for (uint32_t i = 0; i < _type->dimension; i++) {
                if (const IL::DebugSingleValue *member = GetValueFromId(value.values[i], --id)) {
                    return member;
                }
            }

            return nullptr;
        }
        case Backend::IL::TypeKind::Matrix: {
            auto _type = value.type->As<Backend::IL::MatrixType>();
            
            for (uint32_t column = 0; column < _type->columns; column++) {
                for (uint32_t row = 0; row < _type->rows; row++) {
                    if (const IL::DebugSingleValue *member = GetValueFromId(value.values[column * _type->rows + row], --id)) {
                        return member;
                    }
                }
            }

            return nullptr;
        }
    }
}

static uint64_t GetVariableHash(const std::string_view& name, const std::vector<uint8_t>& tinyTypeStream) {
    uint64_t hash = StringCRC32Short(name.data(), name.length());
    CombineHash(hash, BufferCRC32Short(tinyTypeStream.data(), tinyTypeStream.size()));
    return hash;
}

const Backend::IL::Type* DebugFeature::GetInstructionDebugType(const IL::VisitContext &context, const IL::Instruction *instr, const DebugWatchpointMessage& watchpointMessage, WatchpointData& watchpointData, Watchpoint* watchpoint) {
    // Try to reconstruct the debug stack first
    debugEmitter->GetStack(context.program, instr, watchpointData.arena, watchpointData.debugStack);
    
    // Anything?
    if (watchpointData.debugStack.variables.size()) {
        // Variable stream
        MessageStream  variableStream;
        MessageStreamView<DebugWatchpointVariableMetadataMessage> variableView(variableStream);
        
        // Report all variables and their representations
        for (uint64_t i = 0; i < watchpointData.debugStack.variables.size(); i++) {
            const IL::DebugVariable *src = watchpointData.debugStack.variables[i];
            
            // All values
            MessageStream valueStream;

            // Fill value stream, allocate linearly
            uint32_t id = 0;
            FillValueStream(valueStream, src->value, id);

            // Pack the tiny type down
            std::vector<uint8_t> tinyType;
            Backend::IL::Tiny::Pack(src->value.type, tinyType);
            
            // Allocate metadata
            DebugWatchpointVariableMetadataMessage *metadata = variableView.Add(DebugWatchpointVariableMetadataMessage::AllocationInfo {
                .nameLength = src->name.length(),
                .dataTinyTypeCount = tinyType.size(),
                .valuesByteSize = valueStream.GetByteSize()
            });
            
            // Copy over tiny type
            std::memcpy(metadata->dataTinyType.Get(), tinyType.data(), tinyType.size());
            
            // Keep hash around
            uint64_t hash = GetVariableHash(src->name, tinyType);
            watchpointData.variables[hash] = src;
            
            // Copy over the info
            metadata->name.Set(src->name);
            metadata->variableHash = hash;
            metadata->values.Set(valueStream);
        }
        
        // Empty out last stream
        MessageStream metadataStream;
        MessageStreamView<DebugWatchpointMetadataMessage> view(metadataStream);

        // Allocate watchpoint metadata
        auto message = view.Add(DebugWatchpointMetadataMessage::AllocationInfo {
            .variablesByteSize = variableStream.GetByteSize()
        });
        
        // Write out all variables
        message->variables.Set(variableStream);
        message->uid = watchpoint->uid;
        
        // Push metadata
        bridge->GetOutput()->AddStreamAndSwap(metadataStream);
        
        // Assign ids
        watchpointData.variableHash = watchpointMessage.variableHash;
        watchpointData.valueId = watchpointMessage.valueId;
        
        // Find owning variable
        if (auto varIt = watchpointData.variables.find(watchpointData.variableHash); varIt != watchpointData.variables.end()) {
            // Try to find value
            uint32_t idDecrement = watchpointData.valueId;
            if (const IL::DebugSingleValue* value = GetValueFromId(varIt->second->value, idDecrement)) {
                return value->type;
            }
        }
    }

    // Try to get the raw type
    if (IL::ID nonDebugValue = GetInstructionRawDebugValue(context, instr); nonDebugValue != IL::InvalidID) {
        return context.program.GetTypeMap().GetType(nonDebugValue);
    }

    return nullptr;
}

IL::ID DebugFeature::GetInstructionDebugValue(const IL::VisitContext &context, const IL::Instruction* instr, WatchpointData& watchpointData, IL::BasicBlock::Iterator& insertIt) {
    // Emit after the instruction
    IL::Emitter<> emitter(context.program, context.basicBlock, insertIt);

    // Try to reconstruct the source value first
    if (auto varIt = watchpointData.variables.find(watchpointData.variableHash); varIt != watchpointData.variables.end()) {
        // Try to find value
        uint32_t idDecrement = watchpointData.valueId;
        if (const IL::DebugSingleValue* value = GetValueFromId(varIt->second->value, idDecrement)) {
            // Attempt to reconstruct
            if (IL::ID reconstructed = debugEmitter->ReconstructValue(emitter, *value, instr); reconstructed != IL::InvalidID) {
                // Split after the constructed debug value
                insertIt = emitter.GetIterator();
                return reconstructed;
            }
        }
    }

    // Get the raw value
    return GetInstructionRawDebugValue(context, instr);
}

IL::ID DebugFeature::GetInstructionRawDebugValue(const IL::VisitContext &context, const IL::Instruction *instr) {
    // Select the value for debugging
    switch (instr->opCode) {
        default:
            return instr->result;
        case IL::OpCode::Store:
            return instr->As<IL::StoreInstruction>()->value;
        case IL::OpCode::StoreTexture:
            return instr->As<IL::StoreTextureInstruction>()->texel;
        case IL::OpCode::StoreBuffer:
            return instr->As<IL::StoreBufferInstruction>()->value;
        case IL::OpCode::StoreBufferRaw:
            return instr->As<IL::StoreBufferRawInstruction>()->value;
        case IL::OpCode::StorePrimitiveOutput:
            return instr->As<IL::StorePrimitiveOutputInstruction>()->value;
        case IL::OpCode::StoreVertexOutput:
            return instr->As<IL::StoreVertexOutputInstruction>()->value;
        case IL::OpCode::StoreOutput:
            return instr->As<IL::StoreOutputInstruction>()->value;
        case IL::OpCode::BranchConditional:
            return instr->As<IL::BranchConditionalInstruction>()->cond;
    }
}

bool DebugFeature::GetWatchpointFormat(WatchpointData& watchpointData) {
    // Check compression
    switch (watchpointData.hostLayout.compression) {
        default: {
            //  No compression
            break;
        }
        case WatchpointCompression::FPUnorm8888: {
            watchpointData.hostLayout.format = Backend::IL::Format::RGBA8;
            return true;
        }
    }

    // No format
    // TODO[dbg]: This is obviously incomplete
    return false;
}

bool DebugFeature::SupportsKernelType(IL::KernelType kernelType, Watchpoint* watchpoint) {
    switch (watchpoint->captureMode) {
        default: {
            ASSERT(false, "Invalid mode");
            return false;
        }
        case WatchpointCaptureMode::FirstEvent:
        case WatchpointCaptureMode::FirstViewport: {
            // Only some shaders supported for now
            switch (kernelType) {
                default: {
                    ASSERT(false, "Invalid mode");
                    return false;
                }
                case IL::KernelType::None:
                case IL::KernelType::Vertex:
                case IL::KernelType::Geometry:
                case IL::KernelType::Hull:
                case IL::KernelType::Domain:
                case IL::KernelType::Amplification:
                case IL::KernelType::Mesh:
                case IL::KernelType::RayGen:
                case IL::KernelType::RayMiss:
                case IL::KernelType::RayHit:
                case IL::KernelType::Lib: {
                    // Not supported, yet
                    return false;
                }
                case IL::KernelType::Compute:
                case IL::KernelType::Pixel: {
                    // Supported
                    return true;
                }
            }
        }
        case WatchpointCaptureMode::AllEvents: {
            // Always supported
            return true;
        }
    }
}

bool DebugFeature::GetWatchpointDataHostLayout(const IL::VisitContext &context, const IL::Instruction* instr, const Backend::IL::Type* valueType, Watchpoint* watchpoint, WatchpointData& watchpointData) {
    // Type may not be serializable
    if (!IsTypeSerializationSupported(valueType)) {
        return false;
    }

    // Check kernel type support
    auto* kernelType = context.program.GetMetadataMap().GetMetadata<IL::KernelTypeMetadata>(context.program.GetEntryPoint()->GetID());
    if (!SupportsKernelType(kernelType->type, watchpoint)) {
        return false;
    }

    // Supports 8-8-8-8 compression?
    if (watchpoint->captureMode != WatchpointCaptureMode::AllEvents) {
        if (watchpointData.flags & WatchpointFlag::AllowImageFPUNorm8888Compression && SupportsImageFPUnormCompression(instr, valueType)) {
            watchpointData.hostLayout.compression = WatchpointCompression::FPUnorm8888;
        }
    }

    // Try to get the format
    if (GetWatchpointFormat(watchpointData)) {
        // Assume stride
        watchpointData.hostLayout.dataDWordStride = static_cast<uint32_t>(GetSize(watchpointData.hostLayout.format) / sizeof(uint32_t));
    } else {
        // If not relevant, just assume the type
        watchpointData.hostLayout.typeId = valueType->id;

        // Pack the tiny type down
        Backend::IL::Tiny::Pack(valueType, watchpointData.hostLayout.tinyType);

        // TODO[dbg]: I guess we don't need to handle alignment?
        watchpointData.hostLayout.dataDWordStride = static_cast<uint32_t>((GetPODNonAlignedTypeByteSize(valueType) + sizeof(uint32_t) - 1) / sizeof(uint32_t));
    }

    // Host layout supported
    return true;
}

void DebugFeature::GetWatchpointOrderingFirstEvent(const IL::VisitContext &context, IL::Emitter<>& emitter, IL::ShaderStruct<ExecutionInfo>& execution, IL::ShaderBufferStruct<WatchpointHeader>& watchpointHeader, Watchpoint *watchpoint, WatchpointData& watchpointData) {
    auto* kernelType = context.program.GetMetadataMap().GetMetadata<IL::KernelTypeMetadata>(context.program.GetEntryPoint()->GetID());

    // Default init
    watchpointData.firstEvent.staticOrderWidth = emitter.UInt32(0);
    watchpointData.firstEvent.staticOrderHeight = emitter.UInt32(0);
    watchpointData.firstEvent.staticOrderDepth = emitter.UInt32(0);

    IL::ID dwordStride = emitter.UInt32(watchpointData.hostLayout.dataDWordStride);

    // Get work dimensions
    switch (kernelType->type) {
        default: {
            ASSERT(false, "Unexpected type");
            break;
        }
        case IL::KernelType::Pixel: {
            // Determine the thread counts
            watchpointData.firstEvent.staticOrderWidth = execution.Get<&ExecutionInfo::viewport>(emitter, 0);
            watchpointData.firstEvent.staticOrderHeight = execution.Get<&ExecutionInfo::viewport>(emitter, 1);
            watchpointData.firstEvent.staticOrderDepth = emitter.UInt32(1);
            break;
        }
        case IL::KernelType::Compute: {
            auto* kernelWorkgroupSize = context.program.GetMetadataMap().GetMetadata<IL::KernelWorkgroupSizeMetadata>(context.program.GetEntryPoint()->GetID());

            // Get the number of thread groups
            IL::ID threadGroupsX = execution.Get<&ExecutionInfo::dispatch>(emitter, 0);
            IL::ID threadGroupsY = execution.Get<&ExecutionInfo::dispatch>(emitter, 1);
            IL::ID threadGroupsZ = execution.Get<&ExecutionInfo::dispatch>(emitter, 2);

            // Determine the thread counts
            watchpointData.firstEvent.staticOrderWidth = emitter.Mul(threadGroupsX, emitter.UInt32(kernelWorkgroupSize->threadsX));
            watchpointData.firstEvent.staticOrderHeight = emitter.Mul(threadGroupsY, emitter.UInt32(kernelWorkgroupSize->threadsY));
            watchpointData.firstEvent.staticOrderDepth = emitter.Mul(threadGroupsZ, emitter.UInt32(kernelWorkgroupSize->threadsZ));
            break;
        }
    }

    // Determine the max number of dwords
    watchpointData.firstEvent.dwordStreamCount = emitter.Mul(watchpointData.firstEvent.staticOrderWidth, emitter.Mul(watchpointData.firstEvent.staticOrderHeight, emitter.Mul(watchpointData.firstEvent.staticOrderDepth, dwordStride)));

    // Max number of dwords
    IL::ID payloadDWordCount = watchpointHeader.Get<&WatchpointHeader::payloadDWordCount>(emitter);

    /// Is this a dynamic payload?
    watchpointData.firstEvent.isDynamic = emitter.GreaterThan(watchpointData.firstEvent.dwordStreamCount, payloadDWordCount);

    // Select dynamic if we exceed 
    watchpointData.orderType = emitter.Select(
        watchpointData.firstEvent.isDynamic,
        emitter.UInt32(static_cast<uint32_t>(WatchpointDataOrder::Dynamic)),
        emitter.UInt32(static_cast<uint32_t>(WatchpointDataOrder::Static))
    );

    // Indices
    IL::ID x = IL::InvalidID;
    IL::ID y = IL::InvalidID;
    IL::ID z = IL::InvalidID;

    // Get thread indices
    switch (kernelType->type) {
        default: {
            ASSERT(false, "Unexpected type");
            break;
        }
        case IL::KernelType::Pixel: {
            // Get typed dispatch index
            IL::ID pos = emitter.KernelValue(Backend::IL::KernelValue::PixelPosition);

            // Get dimensions
            x = emitter.FloatToUInt32(emitter.Extract(pos, emitter.UInt32(0)));
            y = emitter.FloatToUInt32(emitter.Extract(pos, emitter.UInt32(1)));
            z = emitter.UInt32(0);
            break;
        }
        case IL::KernelType::Compute: {
            // Get typed dispatch index
            IL::ID dtid = emitter.KernelValue(Backend::IL::KernelValue::DispatchThreadID);

            // Get dimensions
            x = emitter.Extract(dtid, emitter.UInt32(0));
            y = emitter.Extract(dtid, emitter.UInt32(1));
            z = emitter.Extract(dtid, emitter.UInt32(2));
            break;
        }
    }

    // Static ordering
    IL::ID staticOrder;
    {
        // z * w * h + y * w + x
        staticOrder = emitter.Mul(z, emitter.Mul(watchpointData.firstEvent.staticOrderWidth, watchpointData.firstEvent.staticOrderHeight));
        staticOrder = emitter.Add(staticOrder, emitter.Mul(y, watchpointData.firstEvent.staticOrderWidth));
        staticOrder = emitter.Add(staticOrder, x);
    }

    // Assume static ordering for now, dynamic exporting happens later
    watchpointData.staticOrder = staticOrder;
    
#if !defined(NDEBUG) && 0
    watchpointHeader.Set<&WatchpointHeader::debugPayloads>(emitter, x, 0);
    watchpointHeader.Set<&WatchpointHeader::debugPayloads>(emitter, y, 1);
    watchpointHeader.Set<&WatchpointHeader::debugPayloads>(emitter, z, 2);
#endif // NDEBUG
}

static void GetWatchpointDataDWords(const IL::VisitContext &context, IL::Emitter<>& emitter, IL::ID value, uint32_t& byteOffset, TrivialStackVector<IL::ID, 16u>& dwords) {
    const Backend::IL::Type *type = context.program.GetTypeMap().GetType(value);

    // Structural
    switch (type->kind) {
        default: {
            break;
        }
        case Backend::IL::TypeKind::Vector: {
            auto typed = type->As<Backend::IL::VectorType>();

            // Get all nested dwords
            for (uint32_t i = 0; i < typed->dimension; i++) {
                GetWatchpointDataDWords(
                    context, emitter, emitter.Extract(value, emitter.UInt32(i)),
                    byteOffset,
                    dwords
                );
            }
            
            return;
        }
        case Backend::IL::TypeKind::Struct: {
            auto typed = type->As<Backend::IL::StructType>();

            // Get all nested dwords
            for (uint32_t i = 0; i < static_cast<uint32_t>(typed->memberTypes.size()); i++) {
                GetWatchpointDataDWords(
                    context, emitter, emitter.Extract(value, emitter.UInt32(i)),
                    byteOffset,
                    dwords
                );
            }
            
            return;
        }
    }

    // TODO: Hmm, >32 bit handling?
    const Backend::IL::Type *encodeType = context.program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType {
        .bitWidth = 32
    });
    
    // Type conversion
    switch (type->kind) {
        default: {
            ASSERT(false, "Unexpected type");
            break;
        }
        case Backend::IL::TypeKind::Bool: {
            value = emitter.Select(value, emitter.UInt32(1), emitter.UInt32(0));
            break;
        }
        case Backend::IL::TypeKind::Int:
        case Backend::IL::TypeKind::FP: {
            // TODO[dbg]: Handle ext
            // TODO[dbg]: double?
            break;
        }
    }

    // Cast to u32
    value = emitter.BitCast(value, encodeType);

    // Get the number of bytes
    uint32_t byteCount = static_cast<uint32_t>(GetPODNonAlignedTypeByteSize(type));

    // Fast path for dword aligned offset and data
    if (byteOffset % sizeof(uint32_t) == 0 && byteCount == sizeof(uint32_t)) {
        uint32_t dword = static_cast<uint32_t>(byteOffset / sizeof(uint32_t));
        dwords[dword] = value;
        byteOffset += sizeof(uint32_t);
        return;
    }

    // Not aligned, write the data out, carefully handling the dword boundaries
    for (uint32_t componentByteOffset = 0; componentByteOffset < byteCount;) {
        uint32_t dword          = static_cast<uint32_t>(byteOffset / sizeof(uint32_t));
        uint32_t dwordBitOffset = static_cast<uint32_t>((byteOffset % sizeof(uint32_t)) * 8);
        uint32_t nextDWord      = dword + 1;
        uint32_t remBytes       = std::min(byteCount - componentByteOffset, static_cast<uint32_t>(nextDWord * sizeof(uint32_t)) - componentByteOffset);
        
        // Take the value, cut out the portion that we're writing
        IL::ID maskedValue = emitter.BitAnd(
            emitter.BitShiftRight(value, emitter.UInt32(static_cast<uint32_t>(componentByteOffset % sizeof(uint32_t)) * 8)),
            emitter.UInt32(std::bit_width(remBytes) - 1)
        );

        // Write the destination dword
        dwords[dword] = emitter.BitOr(dwords[dword], emitter.BitShiftLeft(maskedValue, emitter.UInt32(dwordBitOffset)));

        // Next offset
        componentByteOffset += remBytes;
        byteOffset += remBytes;
    }
}

void DebugFeature::StoreWatchpointDataDWords(const IL::VisitContext &context, IL::Emitter<>& emitter, IL::ID value, Watchpoint* watchpoint, WatchpointData& watchpointData) {
    // Zero init dwords
    TrivialStackVector<IL::ID, 16u> dwords(watchpointData.hostLayout.dataDWordStride);
    for (uint32_t i = 0; i < watchpointData.hostLayout.dataDWordStride; i++) {
        dwords[i] = emitter.UInt32(0);
    }

    // Get the data dwords, handles alignment
    uint32_t byteOffset = 0;
    GetWatchpointDataDWords(context, emitter, value, byteOffset, dwords);

    // Get the data ids
    IL::ID streamLoadID = emitter.Load(context.program.GetShaderDataMap().Get(streamBufferID)->id);
    
    // Finally, write them out
    for (uint32_t i = 0; i < watchpointData.hostLayout.dataDWordStride; i++) {
        IL::ID offset = emitter.Add(watchpointData.payloadDataOffset, emitter.UInt32(i));
        emitter.StoreBuffer(streamLoadID, offset, dwords[i]);
    }
}

void DebugFeature::StoreWatchpointData(const IL::VisitContext &context, IL::Emitter<>& emitter, IL::ID value, Watchpoint* watchpoint, WatchpointData& watchpointData) {    
    // Handle any kind of compression
    switch (watchpointData.hostLayout.compression) {
        default: {
            //  No compression
            break;
        }
        case WatchpointCompression::FPUnorm8888: {
            // Compress the value
            value = CompressFPUnorm8888(context, emitter, value);

            // 255 Alpha
            // TODO[dbg]: Remove this, testing only
            value = emitter.BitOr(value, emitter.UInt32(0xFF << 24));
            break;
        }
    }

    // Finally, store the dwords
    StoreWatchpointDataDWords(context, emitter, value, watchpoint, watchpointData);
}

static uint32_t ShaderInstrumentationHashWideTo32(uint64_t wide) {
    return BufferCRC32Short(&wide, sizeof(wide));
}

IL::BasicBlock::Iterator DebugFeature::InjectWatchpoint(const IL::VisitContext &context, IL::BasicBlock::Iterator it, const DebugWatchpointMessage& watchpointMessage) {
    // TODO[dbg]: Send a message back "nothing to debug!" This shouldn't come from a message

    // Invalidate all dominance analysis, instrumentation changes this all the time
    // TODO[dbg]: We really need proper tracking
    context.function.GetAnalysisMap().Remove<IL::DominatorAnalysis>();

    // Find the relevant watchpoint
    Watchpoint* watchpoint = FindWatchpointNoLock(watchpointMessage.uid);
    if (!watchpoint) {
        return it;
    }

    // Intermediate data
    WatchpointData watchpointData;
    watchpointData.flags = static_cast<WatchpointFlag>(watchpointMessage.flags);
    watchpointData.markerHash32 = watchpointMessage.markerHash32;
    watchpointData.shaderInstrumentationHash32 = ShaderInstrumentationHashWideTo32(context.program.GetShaderInstrumentationHash());

    // Get the value to be debugged
    const Backend::IL::Type *valueType = GetInstructionDebugType(context, it, watchpointMessage, watchpointData, watchpoint);
    if (!valueType) {
        return it;
    }

    // Try to determine the data layout
    // This may fail if there's nothing suitable
    if (!GetWatchpointDataHostLayout(context, it, valueType, watchpoint, watchpointData)) {
        return it;
    }

    /**
     * At this point the watchpoint has been accepted, emitting is allowed
     **/

    // Apply any per-program state
    ApplyWatchpointFlagsToProgram(context, watchpointData);

    // Set name for debugging
    it.block->SetName("Watchpoint.EntryBlock");

    // Splitting/inserting after the debug instruction
    IL::BasicBlock::Iterator insertIt = std::next(it);

    // Get the value to be debugged
    // This may modify the program, so do it after
    IL::ID value = GetInstructionDebugValue(context, it, watchpointData, insertIt);
    ASSERT(value != IL::InvalidID, "Type without value");
    
    // Emit in the interrupt block
    IL::BasicBlock* interruptBlock = context.function.GetBasicBlocks().AllocBlock();
    IL::Emitter<>   emitter(context.program, *interruptBlock);

    // Acquire it logically before, to access some shared findings
    IL::BasicBlock* resumeBlock;
    switch (watchpoint->captureMode) {
        default:
            ASSERT(false, "Invalid capture mode");
            return insertIt;
        case WatchpointCaptureMode::FirstEvent:
        case WatchpointCaptureMode::FirstViewport:
            resumeBlock = AcquireAndAllocateWatchpointFirstEvent(context, insertIt, interruptBlock, watchpoint, watchpointData);
            break;
        case WatchpointCaptureMode::AllEvents:
            resumeBlock = AcquireAndAllocateWatchpointAllEvents(context, insertIt, interruptBlock, watchpoint, watchpointData);
            break;
    }

    // Store the watchpoint data
    StoreWatchpointData(context, emitter, value, watchpoint, watchpointData);
    
    // Branch the watchpoint to resume
    IL::Emitter(context.program, *interruptBlock).Branch(resumeBlock);

    // Instrumentation has passed, keep the layout around
    watchpoint->hostLayoutMap[watchpointData.shaderInstrumentationHash32] = watchpointData.hostLayout;

    // Resume iteration
    return resumeBlock->begin();
}

IL::BasicBlock* DebugFeature::AcquireWatchpointFirstEvent(const IL::VisitContext &context, const IL::BasicBlock::Iterator &insertIt, IL::BasicBlock *watchpointBlock, Watchpoint* watchpoint, WatchpointData& watchpointData) {
    /**
     * First-event optimized acquisition.
     *
     * acq = (header.acquiredExecutionUID == rollingExecutionUID)
     * if (header.acquiredExecutionUID == 0) {
     *   last = AtomicCAS(&header.acquiredExecutionUID, 0, rollingExecutionUID)
     *   
     *   allocated = (last == 0)
     *   if (allocated) {
     *     Export(WatchpointAcquisitionMessage {
     *       .uid = <uid>
     *     });
     *   }
     *   
     *   acq = (allocated || last == rollingExecutionUID)
     * }
     *
     * acq = phi <...>
     */

    // Allocate blocks
    IL::BasicBlock* headerBlock      = context.function.GetBasicBlocks().AllocBlock("Bk.Acquire.Header");
    IL::BasicBlock* casBlock         = context.function.GetBasicBlocks().AllocBlock("Bk.Acquire.CAS");
    IL::BasicBlock* casMerge         = context.function.GetBasicBlocks().AllocBlock("Bk.Acquire.CAS.Merge");
    IL::BasicBlock* resumeBlock      = context.function.GetBasicBlocks().AllocBlock("Bk.Acquire.Resume");
    IL::BasicBlock* exportBlock      = context.function.GetBasicBlocks().AllocBlock("Bk.Acquire.Export");
    IL::BasicBlock* exportMergeBlock = context.function.GetBasicBlocks().AllocBlock("Bk.Acquire.Export.Merge");
    
    // Split the iterator to resume
    // Excluding the iterator itself, since we may want to reference the instruction results
    insertIt.block->Split(resumeBlock, insertIt);

    // Immediately branch to the header
    IL::Emitter(context.program, *insertIt.block).Branch(headerBlock);
    IL::Emitter<> headerEmitter(context.program, *headerBlock);

    // Find the relevant watchpoint
    GetWatchpoint(headerEmitter, watchpoint, watchpointData);

    // Get the current execution
    IL::ShaderStruct<ExecutionInfo> execution(headerEmitter.ExecutionInfo());

    // Get the header
    IL::ShaderBufferStruct<WatchpointHeader> watchpointHeader(context.program.GetShaderDataMap().Get(streamBufferID)->id, watchpointData.headerOffset);

    // Get payload offset
    watchpointData.payloadOffset = watchpointHeader.Get<&WatchpointHeader::payloadDWordOffset>(headerEmitter);

    // Get the ordering, this is used by both the acquire header and export
    GetWatchpointOrderingFirstEvent(context, headerEmitter, execution, watchpointHeader, watchpoint, watchpointData);

    // Read the curent acquired UID
    // This is a regular buffer read, not atomic
    IL::ID acquiredUID = watchpointHeader.Get<&WatchpointHeader::acquiredExecutionUID>(headerEmitter);

    // Get the rolling uid
    IL::ID rollingUID;
    switch (watchpoint->captureMode) {
        default: {
            ASSERT(false, "Invalid capture mode");
            return nullptr;
        }
        case WatchpointCaptureMode::FirstEvent: {
            rollingUID = execution.Get<&ExecutionInfo::rollingExecutionUID>(headerEmitter);
            break;
        }
        case WatchpointCaptureMode::FirstViewport: {
            rollingUID = execution.Get<&ExecutionInfo::rollingViewportUID>(headerEmitter);
            break;
        }
    }
    
    // Acquired states, first check if it's equal to the current UID
    IL::ID acquiredHeader = headerEmitter.Equal(acquiredUID, rollingUID);
    IL::ID acquiredCAS       = IL::InvalidID;

    // See branch
    IL::ID canAllocateHeader = headerEmitter.Equal(acquiredUID, headerEmitter.UInt32(0));

    // Supplied hash?
    if (watchpointData.markerHash32) {
        IL::ID hash32         = headerEmitter.UInt32(watchpointData.markerHash32);
        IL::ID anyHashMatched = headerEmitter.Bool(false);

        // Check if any of the scopes match
        for (uint32_t i = 0; i < kMaxExecutionInfoMarkerCount; i++) {
            anyHashMatched = headerEmitter.Or(
                anyHashMatched,
                headerEmitter.Equal(
                    execution.Get<&ExecutionInfo::markerHashes32>(headerEmitter, i),
                    hash32
                )
            );
        }

        // Must pass both
        canAllocateHeader = headerEmitter.And(canAllocateHeader, anyHashMatched);
    }

    // If not acquired, and it's equal to zero (i.e., unallocated), enter the CAS block
    // With this we've validated that we don't hold the lock and nothing else does.
    // Of course, the cache lines may not represent the real state, but in case of mismatches
    // we'll enter the CAS block anyhow.
    headerEmitter.BranchConditional(
        canAllocateHeader,
        casBlock,
        casMerge,
        IL::ControlFlow::Selection(casMerge)
    );

    // CAS
    {
        IL::Emitter<> casEmitter(context.program, *casBlock);

        // Actually do the CAS
        IL::ID previousValue = watchpointHeader.AtomicCompareExchange<&WatchpointHeader::acquiredExecutionUID>(casEmitter, casEmitter.UInt32(0), rollingUID);

        // Allocated if it was zero
        IL::ID allocatedCAS = casEmitter.Equal(previousValue, casEmitter.UInt32(0));

        // Either we allocated or acquired the UID
        acquiredCAS = casEmitter.Or(allocatedCAS, casEmitter.Equal(previousValue, rollingUID));

        // If allocated, move to the export block
        casEmitter.BranchConditional(allocatedCAS, exportBlock, exportMergeBlock, IL::ControlFlow::Selection(exportMergeBlock));

        // Export
        {
            IL::Emitter<> exportEmitter(context.program, *exportBlock);

            // Write out that the watchpoint was allocated
            // Since streams are per-submission, this is entirely atomic and coherent
            WatchpointAcquisitionMessage::ShaderExport msg;
            msg.chunks |= WatchpointAcquisitionMessage::Chunk::ExtraData;
            msg.uid = exportEmitter.UInt32(watchpoint->uid);
            msg.magic = exportEmitter.UInt32(42);
            msg.extraData.instrumentationHash32 = exportEmitter.UInt32(watchpointData.shaderInstrumentationHash32);
            exportEmitter.Export(exportID, msg);

            // Update the watchpoint header's device data layout
            // Used on the host to resolve ordering
            watchpointHeader.Set<&WatchpointHeader::dataOrder>(exportEmitter, watchpointData.orderType);
            watchpointHeader.Set<&WatchpointHeader::staticWidth>(exportEmitter, watchpointData.firstEvent.staticOrderWidth);
            watchpointHeader.Set<&WatchpointHeader::staticHeight>(exportEmitter, watchpointData.firstEvent.staticOrderHeight);
            watchpointHeader.Set<&WatchpointHeader::staticDepth>(exportEmitter, watchpointData.firstEvent.staticOrderDepth);
            watchpointHeader.Set<&WatchpointHeader::dwordStreamCount>(exportEmitter, watchpointData.firstEvent.dwordStreamCount);

            exportEmitter.Branch(exportMergeBlock);
        }
        
        // Export Merge
        {
            IL::Emitter<> mergeEmitter(context.program, *exportMergeBlock);
            mergeEmitter.Branch(casMerge);
        }
    }

    // Merge
    {
        IL::Emitter<> mergeEmitter(context.program, *casMerge);

        // Merge the inbound acquired states
        IL::ID acquired = mergeEmitter.Phi(headerBlock, acquiredHeader, exportMergeBlock, acquiredCAS);

        // If acquired, do watchpoint stuff, otherwise resume the program as usual
        mergeEmitter.BranchConditional(
            acquired,
            watchpointBlock,
            resumeBlock,
            IL::ControlFlow::Selection(resumeBlock)
        );
    }

    // Iterate again
    return resumeBlock;
}

IL::BasicBlock * DebugFeature::AcquireAndAllocateWatchpointFirstEvent(const IL::VisitContext &context, const IL::BasicBlock::Iterator &insertIt, IL::BasicBlock *watchpointBlock, Watchpoint *watchpoint, WatchpointData &watchpointData) {
    /**
    * acq = acquire()
    * if (acq) {
    *   if (dynamic) {
    *     order = allocDynamic()
    *   }
    *
    *   order = phi <...>
    *   watchpoint
    */

    // Allocate blocks
    IL::BasicBlock* headerBlock  = context.function.GetBasicBlocks().AllocBlock("Bk.Inject.Header");
    IL::BasicBlock* dynamicBlock = context.function.GetBasicBlocks().AllocBlock("Bk.Inject.DynamicAlloc");
    IL::BasicBlock* mergeBlock   = context.function.GetBasicBlocks().AllocBlock("Bk.Inject.Merge");

    // Acquire the watchpoint
    IL::BasicBlock* resumeBlock = AcquireWatchpointFirstEvent(context, insertIt, headerBlock, watchpoint, watchpointData);

    // Header
    IL::ID staticPayloadDataOffset;
    {
        IL::Emitter<> emitter(context.program, *headerBlock);

        // Header offset within the payload
        staticPayloadDataOffset = emitter.Mul(watchpointData.staticOrder, emitter.UInt32(watchpointData.hostLayout.dataDWordStride));

        // Offset by payload offset
        staticPayloadDataOffset = emitter.Add(watchpointData.payloadOffset, staticPayloadDataOffset);
        
        emitter.BranchConditional(watchpointData.firstEvent.isDynamic, dynamicBlock, mergeBlock, IL::ControlFlow::Selection(mergeBlock));
    }

    // Dynamic Allocation
    IL::ID dynamicOrder;
    IL::ID dynamicPayloadDataOffset;
    {
        IL::Emitter<> dynamicEmitter(context.program, *dynamicBlock);

        // Get the header
        IL::ShaderBufferStruct<WatchpointHeader> watchpointHeader(context.program.GetShaderDataMap().Get(streamBufferID)->id, watchpointData.headerOffset);

        // Get dynamic ordering
        dynamicOrder = watchpointHeader.AtomicAdd<&WatchpointHeader::dynamicCounter>(dynamicEmitter, dynamicEmitter.UInt32(1));

        // Limit by available number of dwords
        dynamicOrder = IL::ExtendedEmitter(dynamicEmitter).Min(dynamicOrder, watchpointHeader.Get<&WatchpointHeader::payloadDWordCount>(dynamicEmitter));

        // Header offset within the payload
        IL::ID dynamicHeaderDWordOffset = dynamicEmitter.Mul(dynamicOrder, dynamicEmitter.UInt32(watchpointData.hostLayout.dataDWordStride + WatchpointDynamicHeaderDWordCount));

        // Offset by payload offset
        dynamicHeaderDWordOffset = dynamicEmitter.Add(watchpointData.payloadOffset, dynamicHeaderDWordOffset);

        // Store the dynamic header
        IL::ID streamLoadID = dynamicEmitter.Load(context.program.GetShaderDataMap().Get(streamBufferID)->id);
        dynamicEmitter.StoreBuffer(streamLoadID, dynamicHeaderDWordOffset, watchpointData.staticOrder);

        // Start writing after the header
        dynamicPayloadDataOffset = dynamicEmitter.Add(dynamicHeaderDWordOffset, dynamicEmitter.UInt32(WatchpointDynamicHeaderDWordCount));

        // To merge
        dynamicEmitter.Branch(mergeBlock);
    }

    // Merge
    {
        IL::Emitter<> mergeEmitter(context.program, *mergeBlock);

        // Select the appropriate ordering
        watchpointData.exportOrder = mergeEmitter.Phi(
            headerBlock, watchpointData.staticOrder,
            dynamicBlock, dynamicOrder
        );

        // Select the data offset
        watchpointData.payloadDataOffset = mergeEmitter.Phi(
            headerBlock, staticPayloadDataOffset,
            dynamicBlock, dynamicPayloadDataOffset
        );

        // To the actual watchpoint
        mergeEmitter.Branch(watchpointBlock);
    }

    // OK
    return resumeBlock;
}

IL::BasicBlock * DebugFeature::AcquireAndAllocateWatchpointAllEvents(const IL::VisitContext &context, const IL::BasicBlock::Iterator &insertIt, IL::BasicBlock *interruptBlock, Watchpoint *watchpoint, WatchpointData &watchpointData) {
    /**
     * <instr>
     *
     * if (!header.instrumentationHash32) {
     *   atomicCAS(header.instrumentationHash32, 0, <version>)
     * }
     *
     * if (header.instrumentationHash32 == <version>) {
     *   setup
     *   <interrupt>
     * }
     */

    // Allocate blocks
    IL::BasicBlock* hashHeaderBlock     = context.function.GetBasicBlocks().AllocBlock("Bk.Inject.HashHeader");
    IL::BasicBlock* hashAllocationBlock = context.function.GetBasicBlocks().AllocBlock("Bk.Inject.HashAllocation");
    IL::BasicBlock* hashMergeBlock      = context.function.GetBasicBlocks().AllocBlock("Bk.Inject.HashMerge");
    IL::BasicBlock* setupBlock          = context.function.GetBasicBlocks().AllocBlock("Bk.Inject.Setup");
    IL::BasicBlock* resumeBlock         = context.function.GetBasicBlocks().AllocBlock("Bk.Acquire.Resume");

    // Split the iterator to resume
    // Excluding the iterator itself, since we may want to reference the instruction results
    insertIt.block->Split(resumeBlock, insertIt);
    
    // Immediately branch to the header
    IL::Emitter(context.program, *insertIt.block).Branch(hashHeaderBlock);

    // Shared header
    IL::ShaderBufferStruct<WatchpointHeader> watchpointHeader;
    
    // Hash header
    // Check if we're allocated or not
    {
        IL::Emitter<> emitter(context.program, *hashHeaderBlock);

        // Find the relevant watchpoint
        GetWatchpoint(emitter, watchpoint, watchpointData);
        
        // Get the header
        watchpointHeader = IL::ShaderBufferStruct<WatchpointHeader>(context.program.GetShaderDataMap().Get(streamBufferID)->id, watchpointData.headerOffset);

        // Get payload offset
        watchpointData.payloadOffset = watchpointHeader.Get<&WatchpointHeader::payloadDWordOffset>(emitter);

        // Check if the hash is unallocated
        IL::ID isUnallocatedHash = emitter.Equal(
            watchpointHeader.Get<&WatchpointHeader::shaderInstrumentationHash32>(emitter),
            emitter.UInt32(0)
        );

        // Allocate if need be
        emitter.BranchConditional(isUnallocatedHash, hashAllocationBlock, hashMergeBlock, IL::ControlFlow::Selection(hashMergeBlock));
    }

    // Hash allocation
    // Try to allocate the current hash
    {
        IL::Emitter<> emitter(context.program, *hashAllocationBlock);
        
        // Actually do the CAS
        watchpointHeader.AtomicCompareExchange<&WatchpointHeader::shaderInstrumentationHash32>(emitter, emitter.UInt32(0), emitter.UInt32(watchpointData.shaderInstrumentationHash32));

        // Hack
        watchpointHeader.Set<&WatchpointHeader::payloadDataDWordStride>(emitter,  emitter.UInt32(watchpointData.hostLayout.dataDWordStride));

        // Back to merge
        emitter.Branch(hashMergeBlock);
    }

    // Hash merge
    {
        IL::Emitter<> emitter(context.program, *hashMergeBlock);

        // Check if the hash is matching
        // TOOD: Not thread safe, obviously
        IL::ID isMatchingHash = emitter.Equal(
            watchpointHeader.Get<&WatchpointHeader::shaderInstrumentationHash32>(emitter),
            emitter.UInt32(watchpointData.shaderInstrumentationHash32)
        );

        // Allocate if need be
        emitter.BranchConditional(isMatchingHash, setupBlock, resumeBlock, IL::ControlFlow::Selection(resumeBlock));
    }

    // Setup
    {
        IL::Emitter<> emitter(context.program, *setupBlock);

        // Get dynamic ordering
        IL::ID order = watchpointHeader.AtomicAdd<&WatchpointHeader::dynamicCounter>(emitter, emitter.UInt32(1));

        // Limit by available number of dwords
        order = IL::ExtendedEmitter(emitter).Min(order, watchpointHeader.Get<&WatchpointHeader::payloadDWordCount>(emitter));

        // Header offset within the payload
        IL::ID headerDWordOffset = emitter.Mul(order, emitter.UInt32(watchpointData.hostLayout.dataDWordStride + WatchpointLooseHeaderDWordCount));

        // Offset by payload offset
        headerDWordOffset = emitter.Add(watchpointData.payloadOffset, headerDWordOffset);

        // Store the dynamic header
        IL::ID streamLoadID = emitter.Load(context.program.GetShaderDataMap().Get(streamBufferID)->id);

        // Write out the loose header
        {
            // Get the current execution
            IL::ShaderStruct<ExecutionInfo> execution(emitter.ExecutionInfo());

            // Copy over the full execution info
            for (uint32_t  i = 0; i < kExecutionInfoDWordCount; i++) {
                emitter.StoreBuffer(streamLoadID, emitter.Add(headerDWordOffset, emitter.UInt32(i)), execution.GetDWord(emitter, i));
            }

            // Local thread data
            IL::ID threadX = IL::InvalidID;
            IL::ID threadY = IL::InvalidID;
            IL::ID threadZ = IL::InvalidID;

            // Get the thread indices
            auto* kernelType = context.program.GetMetadataMap().GetMetadata<IL::KernelTypeMetadata>(context.program.GetEntryPoint()->GetID());
            switch (kernelType->type) {
                default: {
                    // Just default to zero for now
                    threadX = emitter.UInt32(0);
                    threadY = emitter.UInt32(0);
                    threadZ = emitter.UInt32(0);
                    break;
                }
                case IL::KernelType::Vertex: {
                    // Report VID
                    threadX = emitter.UInt32(emitter.KernelValue(Backend::IL::KernelValue::VertexID));
                    threadY = emitter.UInt32(0);
                    threadZ = emitter.UInt32(0);
                    break;
                }
                case IL::KernelType::Pixel: {
                    // Get typed dispatch index
                    IL::ID pos = emitter.KernelValue(Backend::IL::KernelValue::PixelPosition);

                    // Get dimensions
                    threadX = emitter.FloatToUInt32(emitter.Extract(pos, emitter.UInt32(0)));
                    threadY = emitter.FloatToUInt32(emitter.Extract(pos, emitter.UInt32(1)));
                    threadZ = emitter.UInt32(0);
                    break;
                }
                case IL::KernelType::Compute: {
                    IL::ID threadId = emitter.KernelValue(Backend::IL::KernelValue::DispatchThreadID);
                    threadX = emitter.Extract(threadId, emitter.UInt32(0));
                    threadY = emitter.Extract(threadId, emitter.UInt32(1));
                    threadZ = emitter.Extract(threadId, emitter.UInt32(2));
                    break;
                }
            }

            // Store the thread indices
            emitter.StoreBuffer(streamLoadID, emitter.Add(headerDWordOffset, emitter.UInt32(IL::MemberDWordOffset<&WatchpointLooseHeader::threadX>())), threadX);
            emitter.StoreBuffer(streamLoadID, emitter.Add(headerDWordOffset, emitter.UInt32(IL::MemberDWordOffset<&WatchpointLooseHeader::threadY>())), threadY);
            emitter.StoreBuffer(streamLoadID, emitter.Add(headerDWordOffset, emitter.UInt32(IL::MemberDWordOffset<&WatchpointLooseHeader::threadZ>())), threadZ);
        }

        // Start writing after the header
        watchpointData.payloadDataOffset = emitter.Add(headerDWordOffset, emitter.UInt32(WatchpointLooseHeaderDWordCount));
        watchpointData.exportOrder = order;

        // To merge
        emitter.Branch(interruptBlock);
    }

    // OK
    return resumeBlock;
}

void DebugFeature::CreateAndUpdatePayload(Watchpoint &watchpoint) {
    // Allocate the underlying memory
    watchpoint.streamAllocation = buddyAllocator.Allocate(watchpoint.streamSize);

    // Setup the default header
    watchpoint.header.payloadDWordOffset = static_cast<uint32_t>(watchpoint.streamAllocation.offset / sizeof(uint32_t));
    watchpoint.header.payloadDWordCount  = static_cast<uint32_t>(watchpoint.streamSize / sizeof(uint32_t));

    // Capture mode modifiers
    switch (watchpoint.captureMode) {
        default:
            break;
        case WatchpointCaptureMode::AllEvents:
            watchpoint.header.dataOrder = WatchpointDataOrder::Loose;
            break;
    }

    // Map the relevant times for the range
    tileResidencyAllocator.Allocate(
        watchpoint.streamAllocation.offset,
        watchpoint.streamAllocation.length
    );
    
    // Update the header when possible
    watchpoint.pendingTransferHeader = true;

    // Create streaming counter-part
    //  If we're using predicates, we can rely on a fully host resident buffer.
    //  However, if using indirect copies, we need a host visible buffer, which has the unfortunate
    //  side effect of residing in a slower memory type, I suspect using page-guards or similar mechanisms,
    //  which is incredibly slow host side.
    watchpoint.hostStreamingBuffer = shaderDataHost->CreateBuffer(ShaderDataBufferInfo {
        .elementCount = watchpoint.streamAllocation.length + sizeof(WatchpointHeader),
        .format = Backend::IL::Format::R8UInt,
        .flagSet = 
            deviceCapabilityTable.supportsPredicates ? 
            ShaderDataBufferFlag::Host : 
            ShaderDataBufferFlag::HostVisible | ShaderDataBufferFlag::NonDescriptor
    }, "DebugStreamHost");
}

DebugFeature::Watchpoint * DebugFeature::FindWatchpointNoLock(uint32_t uid) {
    // Find the relevant watchpoint
    for (Watchpoint& _candidate : watchpoints) {
        if (_candidate.uid == uid) {
            return &_candidate;
        }
    }

    // Not found
    return nullptr;
}

void DebugFeature::ApplyWatchpointFlagsToProgram(const IL::VisitContext &context, const WatchpointData &watchpointData) {
    // Using early depth stencil?
    if (watchpointData.flags & WatchpointFlag::EarlyDepthStencil) {
        auto* kernelType = context.program.GetMetadataMap().GetMetadata<IL::KernelTypeMetadata>(context.program.GetEntryPoint()->GetID());

        // Only for pixel shaders
        if (kernelType->type == IL::KernelType::Pixel) {
            context.program.GetMetadataMap().AddMetadata(
                context.program.GetEntryPoint()->GetID(),
                IL::MetadataType::EarlyDepthStencil
            );
        }
    }
}

void DebugFeature::GetWatchpoint(IL::Emitter<> &emitter, Watchpoint *watchpoint, WatchpointData& watchpointData) {
    // Set the dword offset
    watchpointData.headerOffset = emitter.UInt32(watchpoint->uid * WatchpointHeaderDWordCount);
}

void DebugFeature::GetWatchpoint(IL::Emitter<> &emitter, DebugWatchpointMessage watchpoint, WatchpointData& watchpointData) {
    std::lock_guard guard(mutex);

    // Find the relevant watchpoint
    Watchpoint* candidate = FindWatchpointNoLock(watchpoint.uid);

    // Shouldn't happen
    if (!candidate) {
        ASSERT(false, "Invalid candidate");
        return;
    }

    // Set the dword offset
    GetWatchpoint(emitter, candidate, watchpointData);
}

FeatureInfo DebugFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Debug";
    info.description = "";
    return info;
}
