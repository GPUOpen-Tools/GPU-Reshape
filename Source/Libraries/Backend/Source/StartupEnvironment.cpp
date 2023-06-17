// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

#include <Backend/StartupEnvironment.h>
#include <Backend/EnvironmentKeys.h>

// Common
#include <Common/Alloca.h>
#include <Common/FileSystem.h>
#include <Common/GlobalUID.h>

// Std
#include <filesystem>
#include <fstream>

using namespace Backend;

bool StartupEnvironment::LoadFromEnvironment(MessageStream& stream) {
    // Try to get length
    uint64_t length{0};
    if (getenv_s(&length, nullptr, 0u, kStartupEnvironmentKey) || !length) {
        return true;
    }

    // Get path data
    auto* path = ALLOCA_ARRAY(char, length);
    if (getenv_s(&length, path, length, kStartupEnvironmentKey)) {
        return false;
    }

    // Invalid state if doesn't exist
    if (!std::filesystem::exists(path)) {
        return false;
    }

    // Open file and get size
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Assign schema
    stream.ValidateOrSetSchema(OrderedMessageSchema::GetSchema());
    
    // Try to read stream
    if (!file.read(reinterpret_cast<char*>(stream.ResizeData(size)), size)) {
        return false;
    }

    // OK
    return true;
}

std::string StartupEnvironment::WriteEnvironment(const MessageStream& stream) {
    // Create new environment path
    std::filesystem::path path = GetIntermediatePath("StartupEnvironment") / (GlobalUID::New().ToString() + ".env");

    // Write to output
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(stream.GetDataBegin()), stream.GetByteSize());

    // OK
    return path.string();
}
