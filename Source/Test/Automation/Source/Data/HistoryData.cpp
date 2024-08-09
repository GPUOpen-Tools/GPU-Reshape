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

// Test
#include <Test/Automation/Data/HistoryData.h>

// Common
#include <Common/FileSystem.h>

// Std
#include <fstream>

void HistoryData::Restore() {
    std::filesystem::path path = GetIntermediatePath("Tests") / "History.blob";

    // Try to open history
    std::ifstream stream(path, std::ios_base::binary);
    if (!stream.good()) {
        return;
    }

    // Read number of tags
    uint64_t count;
    stream.read(reinterpret_cast<char*>(&count), sizeof(count));

    // Read all tags
    for (uint64_t i = 0; i < count; i++) {
        uint64_t tag;
        stream.read(reinterpret_cast<char*>(&tag), sizeof(tag));

        completedTags.insert(tag);
    }
}

void HistoryData::Flush() {
    std::filesystem::path path = GetIntermediatePath("Tests") / "History.blob";
    
    // Try to open history
    std::ofstream stream(path, std::ios_base::binary);
    if (!stream.good()) {
        return;
    }

    // Write number of tags
    uint64_t count = completedTags.size();
    stream.write(reinterpret_cast<const char*>(&count), sizeof(count));

    // Write all tags
    for (uint64_t tag : completedTags) {
        stream.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
    }
}
