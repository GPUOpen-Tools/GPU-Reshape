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

#include <Backends/Vulkan/Compiler/ShaderCompilerDebug.h>
#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>
#include <Backends/Vulkan/Compiler/SpvModule.h>
#include <Backends/Vulkan/Compiler/SpvDebugMap.h>
#include <Backends/Vulkan/Compiler/SpvSourceMap.h>

// Backend
#include <Backend/IL/Program.h>
#include <Backend/IL/PrettyPrint.h>

// SPIRV-Tools
#include <spirv-tools/libspirv.hpp>

// Common
#include <Common/FileSystem.h>
#include <Common/GlobalUID.h>

// Std
#include <fstream>

// System
#if defined(_MSC_VER)
#   include <Windows.h>
#endif

ShaderCompilerDebug::ShaderCompilerDebug(InstanceDispatchTable *table) : table(table) {
    path = GetIntermediateDebugPath();

    // Append engine
    if (table->applicationInfo->pEngineName) {
        path /= table->applicationInfo->pEngineName;
    }

    // Append application
    if (table->applicationInfo->pApplicationName) {
        path /= table->applicationInfo->pApplicationName;
    }

    // Append API
    path /= "Vulkan";

    // Clear the sub-tree once per process
    if (static bool once = false; !once) {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
        once = true;
    }

    // Ensure the tree exists
    CreateDirectoryTree(path);
}

ShaderCompilerDebug::~ShaderCompilerDebug() {
    if (tools) {
        destroy(tools, allocators);
    }
}

static void OnValidationMessage(spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) {
    // Determine stream
    FILE* out{nullptr};
    switch (level) {
        case SPV_MSG_FATAL:
        case SPV_MSG_INTERNAL_ERROR:
        case SPV_MSG_ERROR:
            out = stderr;
            break;
        case SPV_MSG_WARNING:
        case SPV_MSG_INFO:
        case SPV_MSG_DEBUG:
            out = stdout;
            break;
    }

#ifdef _MSC_VER
    OutputDebugString(message);
#endif

    // Print it
    fprintf(out, "%s\n", message);
}

bool ShaderCompilerDebug::Install() {
    // Create tools
    tools = new (allocators) spvtools::SpirvTools(SPV_ENV_VULKAN_1_2);

    // Set the consumer
    tools->SetMessageConsumer(OnValidationMessage);

    // OK
    return true;
}

bool ShaderCompilerDebug::Validate(const uint32_t* spirvCode, uint64_t spirvSize) {
    // Ensure validation is sequential for easier debugging
    std::lock_guard guard(sharedLock);

    // Validate first
    // Note that the binary size operand is dword count, which is confusing
    if (tools->Validate(spirvCode, spirvSize)) {
        return true;
    }

    // Failed validation, disassemble the SPIRV
    std::string disassembled;
    if (!tools->Disassemble(spirvCode, spirvSize, &disassembled)) {
        return false;
    }

#ifdef _MSC_VER
    OutputDebugString(disassembled.c_str());
#endif

    fprintf(stderr, "%s\n", disassembled.c_str());
    return false;
}

std::filesystem::path ShaderCompilerDebug::AllocatePath(const std::string_view &view) {
    std::filesystem::path shaderPath = path;

    // Allocate path
    std::string guid = GlobalUID::New().ToString();

    // Compose base path
    if (!view.empty()) {
        shaderPath /= view;
        shaderPath += "." + guid;
    } else {
        shaderPath /= guid;
    }

    return shaderPath;
}

std::filesystem::path ShaderCompilerDebug::AllocatePath(SpvModule *module) {
    std::filesystem::path shaderPath = path;

    // New guid for shader
    std::string guid = module->GetInstrumentationGUID().ToString();

    // Optional shader filename
    std::filesystem::path filename = module->GetSourceMap()->GetFilename();

    // Compose base path
    if (!filename.empty()) {
        shaderPath /= filename.filename().c_str();
        shaderPath += "." + guid;
    } else {
        shaderPath /= guid;
    }

    return shaderPath;
}

void ShaderCompilerDebug::Add(const std::filesystem::path& basePath, const std::string_view &category, SpvModule *module, const void *spirvCode, uint64_t spirvSize) {
    std::string categoryPath = basePath.string();
    categoryPath.append(".");
    categoryPath.append(category);

    // Module printing
    std::ofstream outIL(categoryPath + ".module.txt");
    if (outIL.is_open()) {
        IL::PrettyPrint(*module->GetProgram(), outIL);
        outIL.close();
    }

    // SPIR-V printing
    std::ofstream outSPIRV(categoryPath + ".spirv", std::ios_base::binary);
    if (outSPIRV.is_open()) {
        outSPIRV.write(reinterpret_cast<const char *>(spirvCode), spirvSize);
        outSPIRV.close();
    }

    // SPIR-V disassembly printing
    std::ofstream outSPIRVDis(categoryPath + ".spirv.txt");
    if (outSPIRVDis.is_open()) {
        std::string disassembled;
        if (!tools->Disassemble(static_cast<const uint32_t*>(spirvCode), spirvSize / sizeof(uint32_t), &disassembled)) {
            outSPIRVDis << "Failed to disassemble SPIRV\n";
        }

        outSPIRVDis.write(disassembled.data(), disassembled.length());
        outSPIRVDis.close();
    }
}
