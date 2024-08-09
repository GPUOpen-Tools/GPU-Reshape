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

// Common
#include <Common/IComponent.h>

// Std
#include <set>

class HistoryData : public TComponent<HistoryData> {
public:
    COMPONENT(HistoryData);

    /// Restore all history
    void Restore();

    /// Flush current history
    void Flush();

    /// Mark a tag as completed
    void Complete(uint64_t tag) {
        completedTags.insert(tag);
        Flush();
    }

    /// Check if a tag is completed
    bool IsCompleted(uint64_t tag) const {
        return completedTags.contains(tag);
    }

    /// Check if a tag is completed, or add it
    /// \return true if tag was already completed
    bool IsCompletedOrAdd(uint64_t tag) {
        bool completed = IsCompleted(tag);
        Complete(tag);
        return completed;
    }

private:
    /// All opaque tags that have been completed
    std::set<uint64_t> completedTags;
};
