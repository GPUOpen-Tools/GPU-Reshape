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
#include <Backend/ShaderData/ShaderData.h>

// Common
#include <Common/Assert.h>
#include <Common/Containers/TrivialStackVector.h>

// Std
#include <cstdint>

struct EventDataStack {
    /// Identifier to offset table
    using RemappingTable = TrivialStackVector<uint32_t, 16u>;

    /// Maximum number of dwords
    static constexpr uint32_t kMaxDWordCount = 64;

    /// Set the remapping table
    /// \param table identifier to offset table
    void SetRemapping(const RemappingTable& table) {
        remappingTable.Resize(table.Size());

        // Copy data
        std::memcpy(remappingTable.Data(), table.Data(), table.Size() * sizeof(uint32_t));
    }

    /// Set the data for a given id
    /// \param did data id
    /// \param value dword to be set
    void Set(ShaderDataID did, uint32_t value) {
        SetAtOffset(remappingTable[did], value);
    }

    /// Flush all graphics work
    void FlushGraphics() {
        graphicsDirtyMask = 0x0;
    }

    /// Flush all compute work
    void FlushCompute() {
        computeDirtyMask = 0x0;
    }

    /// Flush all work
    void Flush() {
        graphicsDirtyMask = 0x0;
        computeDirtyMask = 0x0;
    }

    /// Get the current graphics dirty mask
    uint64_t GetGraphicsDirtyMask() const {
        return graphicsDirtyMask;
    }

    /// Get the current compute dirty mask
    uint64_t GetComputeDirtyMask() const {
        return computeDirtyMask;
    }

    /// Get the underlying dword data
    const uint32_t* GetData() const {
        return dwords;
    }

private:
    /// Set the data at an offset
    /// \param offset dword offset
    /// \param value dword value
    void SetAtOffset(uint32_t offset, uint32_t value) {
        ASSERT(offset < kMaxDWordCount, "Event offset out of bounds");

        // Dirty!
        graphicsDirtyMask |= (1ull << offset);
        computeDirtyMask |= (1ull << offset);

        dwords[offset] = value;
    }

private:
    /// Pipeline masks, TODO: I don't like revealing the underlying principles here!
    uint64_t graphicsDirtyMask{0x0};
    uint64_t computeDirtyMask{0x0};

    /// DWord data
    uint32_t dwords[kMaxDWordCount]{};

    /// Current remapper
    TrivialStackVector<uint32_t, 16u> remappingTable;
};
