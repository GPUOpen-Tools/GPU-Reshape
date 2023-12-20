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

#include <Backend/StartupEnvironment.h>
#include <Backend/EnvironmentKeys.h>

// Common
#include <Common/Alloca.h>
#include <Common/FileSystem.h>
#include <Common/GlobalUID.h>
#include <Common/String.h>

// Std
#include <filesystem>
#include <fstream>

// Json
#include <nlohmann/json.hpp>

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

    // Write offset
    const size_t offset = stream.GetByteSize();
    
    // Try to read stream
    if (!file.read(reinterpret_cast<char*>(stream.ResizeData(offset + size) + offset), size)) {
        return false;
    }

    // OK
    return true;
}

bool StartupEnvironment::LoadFromConfig(MessageStream &stream) {
    // Expected path
    std::string path = (GetIntermediatePath("Settings") / "ApplicationStartupEnvironments.json").string();

    // Present?
    if (!PathExists(path)) {
        return false;
    }
    
    /**
     * Structured startup environment map. The actual startup data is kept in a separate file to keep the indexing
     * json small for each application.
     *
     * Example
     * {
     *   "Applications":
     *   {
     *     "AppName": "ABCD.env"
     *   }
     * }
     */

    // Try to open file as text
    std::ifstream in(path);
    if (!in.good()) {
        return false;
    }

    // Parse contents
    nlohmann::json json{};
    in >> json;

    // Must have Application key
    if (!json.contains("Applications")) {
        return false;
    }

    // Get the top-level executable name
    std::string executableName = GetCurrentExecutableName();

    // Find a relevant key
    std::string startupEnvironmentKey;
    for (auto&& app : json["Applications"].items()) {
        if (std::isearch(executableName, app.key()) == executableName.end()) {
            continue;
        }

        // Set as candidate
        startupEnvironmentKey = app.value();
        break;
    }

    // No candidates?
    if (startupEnvironmentKey.empty()) {
        return false;
    }

    // Create new environment path
    std::filesystem::path startupEnvironmentPath = GetIntermediatePath("StartupEnvironment") / startupEnvironmentKey;
    
    // Open file
    std::ifstream file(startupEnvironmentPath, std::ios::binary | std::ios::ate);
    if (!file.good()) {
        return false;
    }

    // Determine size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Assign schema
    stream.ValidateOrSetSchema(OrderedMessageSchema::GetSchema());
    
    // Write offset
    const size_t offset = stream.GetByteSize();
    
    // Try to read stream
    if (!file.read(reinterpret_cast<char*>(stream.ResizeData(offset + size) + offset), size)) {
        return false;
    }

    // OK
    return true;
}

std::string StartupEnvironment::WriteEnvironment(const MessageStream& stream) {
    // Create new environment path
    std::filesystem::path path = GetIntermediatePath("LaunchEnvironment") / (GlobalUID::New().ToString() + ".env");

    // Write to output
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(stream.GetDataBegin()), stream.GetByteSize());

    // OK
    return path.string();
}
