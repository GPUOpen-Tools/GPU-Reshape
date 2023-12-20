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

#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/InstrumentationInfo.h>
#include <Backends/Vulkan/States/ShaderModuleInstrumentationKey.h>

// Common
#include <Common/Containers/ReferenceObject.h>

// Deep Copy
#include <Backends/Vulkan/DeepCopyObjects.Gen.h>

// Std
#include <mutex>
#include <atomic>
#include <map>

// Forward declarations
struct DeviceDispatchTable;
struct PipelineState;
class SpvModule;

struct ShaderModuleState : public ReferenceObject {
    /// Reference counted destructor
    virtual ~ShaderModuleState();

    /// Add an instrument to this module
    /// \param featureBitSet the enabled feature set
    /// \param module the module in question
    void AddInstrument(const ShaderModuleInstrumentationKey& key, VkShaderModule module) {
        ASSERT(key.featureBitSet, "Invalid instrument addition");

        std::lock_guard lock(mutex);
        instrumentObjects[key] = module;
    }

    /// Get an instrument
    /// \param featureBitSet the enabled feature set
    /// \return nullptr if not found
    VkShaderModule GetInstrument(const ShaderModuleInstrumentationKey& key) {
        if (!key.featureBitSet) {
            return object; 
        }

        // Instrumented request
        std::lock_guard lock(mutex);
        auto&& it = instrumentObjects.find(key);
        if (it == instrumentObjects.end()) {
            return nullptr;
        }

        return it->second;
    }

    /// Check if instrument is present
    /// \param featureBitSet the enabled feature set
    /// \return false if not found
    bool HasInstrument(const ShaderModuleInstrumentationKey& key) {
        if (!key.featureBitSet) {
            return true; 
        }
        
        std::lock_guard lock(mutex);
        return instrumentObjects.count(key) > 0;
    }

    bool Reserve(const ShaderModuleInstrumentationKey& key) {
        ASSERT(key.featureBitSet, "Invalid instrument reservation");
    
        std::lock_guard lock(mutex);
        auto&& it = instrumentObjects.find(key);
        if (it == instrumentObjects.end()) {
            instrumentObjects[key] = nullptr;
            return true;
        }

        return false;
    }

    /// User module
    ///  ! May be nullptr if the top shader module has been destroyed
    VkShaderModule object{VK_NULL_HANDLE};

    /// Backwards reference
    DeviceDispatchTable* table;

    /// Recreation info
    VkShaderModuleCreateInfoDeepCopy createInfoDeepCopy;

    /// SPIRV module of the originating shader
    ///    ! On demand, may be nullptr
    SpvModule* spirvModule{nullptr};

    /// Instrumentation info
    InstrumentationInfo instrumentationInfo;

    /// Instrumented objects lookup
    /// TODO: How do we manage lifetimes here?
    std::map<ShaderModuleInstrumentationKey, VkShaderModule> instrumentObjects;

    /// Module specific lock
    std::mutex mutex;

    /// Unique identifier, unique for the type
    uint64_t uid;
};
