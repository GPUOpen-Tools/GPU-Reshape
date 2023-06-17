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

#include <Backends/DX12/Compiler/ShaderCompilerDebug.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Compiler/DXModule.h>

// Backend
#include <Backend/IL/Program.h>
#include <Backend/IL/PrettyPrint.h>

// Common
#include <Common/FileSystem.h>
#include <Common/GlobalUID.h>

// Std
#include <fstream>

ShaderCompilerDebug::ShaderCompilerDebug() {
    path = GetIntermediateDebugPath();

    // Append engine
    path /= "Unknown";

    // Append application
    path /= GetCurrentExecutableName();

    // Append API
    path /= "DX12";

    // Clear the sub-tree once per process
    if (static bool once = false; !once) {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
        once = true;
    }

    // Ensure the tree exists
    CreateDirectoryTree(path);
}

std::filesystem::path ShaderCompilerDebug::AllocatePath(DXModule *module) {
    std::filesystem::path shaderPath = path;

    // New guid for shader
    std::string guid = module->GetInstrumentationGUID().ToString();

    // TODO: Append filename when you have it
    shaderPath /= guid;

    return shaderPath;
}

std::filesystem::path ShaderCompilerDebug::AllocatePath(const std::string_view& name) {
    std::filesystem::path shaderPath = path;

    // New guid for shader
    std::string guid = GlobalUID::New().ToString();

    shaderPath /= name;
    shaderPath /= guid;

    return shaderPath;
}

void ShaderCompilerDebug::Add(const std::filesystem::path& basePath, const std::string_view &category, DXModule *module) {
    std::string categoryPath = basePath.string();
    categoryPath.append(".");
    categoryPath.append(category);

    // Module printing
    std::ofstream outIL(categoryPath + ".module.txt");
    if (outIL.is_open()) {
        IL::PrettyPrint(*module->GetProgram(), outIL);
        outIL.close();
    }
}
