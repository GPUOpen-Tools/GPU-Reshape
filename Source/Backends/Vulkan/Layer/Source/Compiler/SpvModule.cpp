#include <Backends/Vulkan/Compiler/SpvModule.h>
#include <Backends/Vulkan/Compiler/SpvInstruction.h>
#include <Backends/Vulkan/Compiler/SpvStream.h>
#include <Backends/Vulkan/Compiler/SpvRelocationStream.h>
#include <Backends/Vulkan/Compiler/SpvDebugMap.h>
#include "Backends/Vulkan/Compiler/SpvSourceMap.h"

SpvModule::SpvModule(const Allocators &allocators, uint64_t shaderGUID) : allocators(allocators), shaderGUID(shaderGUID) {
}

SpvModule::~SpvModule() {
    destroy(program, allocators);
    destroy(typeMap, allocators);

    // Check if shallow
    if (!parent) {
        destroy(debugMap, allocators);
        destroy(sourceMap, allocators);
    }
}

SpvModule *SpvModule::Copy() const {
    auto *module = new(allocators) SpvModule(allocators, shaderGUID);
    module->header = header;
    module->spirvProgram = spirvProgram;
    module->program = program->Copy();
    module->typeMap = typeMap->Copy(&module->program->GetTypeMap());
    module->metaData = metaData;
    module->debugMap = debugMap;
    module->sourceMap = sourceMap;
    module->idMap = idMap;
    module->parent = this;
    return module;
}
