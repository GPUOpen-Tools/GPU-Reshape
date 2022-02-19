#include <Backends/Vulkan/Compiler/ShaderCompilerDebug.h>
#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>
#include <Backends/Vulkan/Compiler/SpvModule.h>
#include <Backends/Vulkan/Compiler/SpvDebugMap.h>
#include <Backends/Vulkan/Compiler/SpvSourceMap.h>

// Backend
#include <Backend/IL/Program.h>
#include <Backend/IL/PrettyPrint.h>

// Common
#include <Common/FileSystem.h>
#include <Common/GlobalUID.h>

// Std
#include <fstream>

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

    // Clear the sub-tree
    std::filesystem::remove_all(path);

    // Ensure the tree exists
    CreateDirectoryTree(path);
}

bool ShaderCompilerDebug::Install() {
    // OK
    return true;
}

bool ShaderCompilerDebug::Validate(SpvModule *module) {
    return true;
}

std::filesystem::path ShaderCompilerDebug::AllocatePath(SpvModule *module) {
    std::filesystem::path shaderPath = path;

    // New guid for shader
    std::string guid = GlobalUID::New().ToString();

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

void ShaderCompilerDebug::Add(const std::filesystem::path& basePath, const std::string_view &category, SpvModule *module, const void *spirvCode, uint32_t spirvSize) {
    std::string categoryPath = basePath.string();
    categoryPath.append(".");
    categoryPath.append(category);

    // Module printing
    std::ofstream outIL(categoryPath + ".module.txt");
    if (outIL.is_open()) {
        IL::PrettyPrint(*module->GetProgram(), outIL);
    }

    // SPIR-V printing
    std::ofstream outSPIRV(categoryPath + ".spirv");
    if (outSPIRV.is_open()) {
        outSPIRV.write(reinterpret_cast<const char *>(spirvCode), spirvSize);
    }
}
