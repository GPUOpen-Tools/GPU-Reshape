#include <Backends/Vulkan/Compiler/SpvModule.h>
#include <Backends/Vulkan/Compiler/SpvInstruction.h>
#include <Backends/Vulkan/Compiler/SpvStream.h>
#include <Backends/Vulkan/Compiler/SpvRelocationStream.h>

SpvModule::SpvModule(const Allocators &allocators) : allocators(allocators) {
}

SpvModule::~SpvModule() {
    destroy(program, allocators);
    destroy(typeMap, allocators);
}

SpvModule *SpvModule::Copy() const {
    auto *module = new(allocators) SpvModule(allocators);
    module->header = header;
    module->spirvProgram = spirvProgram;
    module->program = program->Copy();
    module->typeMap = typeMap->Copy();
    return module;
}
