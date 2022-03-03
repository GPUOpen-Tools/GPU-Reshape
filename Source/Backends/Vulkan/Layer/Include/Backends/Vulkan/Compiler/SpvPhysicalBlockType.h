#pragma once

// Std
#include <cstdint>

enum class SpvPhysicalBlockType : uint8_t {
    Capability,
    Extension,
    ExtensionImport,
    MemoryModel,
    EntryPoint,
    ExecutionMode,
    DebugStringSource,
    DebugName,
    DebugModuleProcessed,
    Annotation,
    TypeConstantVariable,
    Function,
    Count
};
