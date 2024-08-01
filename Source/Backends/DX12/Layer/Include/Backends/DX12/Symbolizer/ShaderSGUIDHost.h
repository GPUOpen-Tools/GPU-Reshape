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

#pragma once

// Backend
#include <Backend/IShaderSGUIDHost.h>

// Common
#include <Common/Containers/Vector.h>

// Std
#include <vector>
#include <unordered_map>
#include <mutex>

// Forward declarations
struct DeviceState;
class IDXDebugModule;
class IBridge;

class ShaderSGUIDHost final : public IShaderSGUIDHost {
public:
    explicit ShaderSGUIDHost(DeviceState* table);

    /// Install this host
    /// \return success state
    bool Install();

    /// Commit all messages
    /// \param bridge
    void Commit(IBridge* bridge);

    /// Overrides
    ShaderSGUID Bind(const IL::Program &program, const IL::BasicBlock::ConstIterator& instruction) override;
    ShaderSourceMapping GetMapping(ShaderSGUID sguid) override;
    std::string_view GetSource(ShaderSGUID sguid) override;
    std::string_view GetSource(const ShaderSourceMapping &mapping) override;

private:
    /// Cached entry maps
    struct ShaderEntry {
        std::unordered_map<ShaderSourceMapping, ShaderSourceMapping> mappings;
    };

private:
    /// Parent device
    DeviceState* device;

    /// Shared lock
    std::mutex mutex;

    /// All guid to shader entries
    std::unordered_map<uint64_t, ShaderEntry> shaderEntries;

    /// Current allocation counter
    ShaderSGUID counter{0};

    /// Free'd indices to be used immediately
    Vector<ShaderSGUID> freeIndices;

    /// Reverse sguid lookup
    Vector<ShaderSourceMapping> sguidLookup;

    /// All pending bridge submissions
    Vector<ShaderSGUID> pendingSubmissions;
};
