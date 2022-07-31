#include <Backends/Vulkan/Compiler/SpvModule.h>
#include <Backends/Vulkan/Compiler/SpvInstruction.h>
#include <Backends/Vulkan/Compiler/SpvStream.h>
#include <Backends/Vulkan/Compiler/SpvRelocationStream.h>
#include <Backends/Vulkan/Compiler/SpvDebugMap.h>
#include <Backends/Vulkan/Compiler/SpvSourceMap.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>

SpvModule::SpvModule(const Allocators &allocators, uint64_t shaderGUID, const GlobalUID& instrumentationGUID) : allocators(allocators), shaderGUID(shaderGUID), instrumentationGUID(instrumentationGUID) {
#if SHADER_COMPILER_DEBUG
    instrumentationGUIDName = instrumentationGUID.ToString();
#endif
}

SpvModule::~SpvModule() {
    destroy(program, allocators);
    destroy(physicalBlockTable, allocators);
}

SpvModule *SpvModule::Copy() const {
    auto *module = new(allocators) SpvModule(allocators, shaderGUID, instrumentationGUID);
    module->spirvProgram = spirvProgram;
    module->parent = this;

    // Copy program
    module->program = program->Copy();

    // Create physical block table
    module->physicalBlockTable = new(allocators) SpvPhysicalBlockTable(allocators, *module->program);
    {
        physicalBlockTable->CopyTo(*module->physicalBlockTable);
    }

    // OK
    return module;
}

bool SpvModule::ParseModule(const uint32_t *code, uint32_t wordCount) {
    // Create new program
    program = new(allocators) IL::Program(allocators, shaderGUID);

    // Capatility table
    IL::CapabilityTable& caps = program->GetCapabilityTable();
    caps.hasControlFlow = true;

    // Create physical block table
    physicalBlockTable = new(allocators) SpvPhysicalBlockTable(allocators, *program);

    // Attempt to parse the block table
    if (!physicalBlockTable->Parse(code, wordCount)) {
        return false;
    }

    // OK
    return true;
}

bool SpvModule::Recompile(const uint32_t *code, uint32_t wordCount, const SpvJob& job) {
    for (IL::Function* fn : program->GetFunctionList()) {
        if (!fn->ReorderByDominantBlocks(true)) {
            return false;
        }
    }

    // Try to recompile for the given job
    if (!physicalBlockTable->Compile(job)) {
        return false;
    }

    // Stitch to the program
    physicalBlockTable->Stitch(spirvProgram);

    // OK!
    return true;
}

const SpvSourceMap *SpvModule::GetSourceMap() const {
    if (!physicalBlockTable) {
        return nullptr;
    }

    return &physicalBlockTable->debugStringSource.sourceMap;
}
