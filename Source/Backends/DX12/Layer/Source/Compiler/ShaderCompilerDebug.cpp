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
