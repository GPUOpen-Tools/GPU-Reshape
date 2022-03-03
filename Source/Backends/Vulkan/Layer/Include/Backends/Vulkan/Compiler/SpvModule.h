#pragma once

// Common
#include <Common/Assert.h>

// Std
#include <set>

// Backend
#include <Backend/IL/Program.h>
#include <Backend/IL/Emitter.h>

// Layer
#include <Backends/Vulkan/Compiler/SpvTypeMap.h>
#include <Backends/Vulkan/Compiler/SpvJob.h>
#include <Backends/Vulkan/States/ShaderModuleInstrumentationKey.h>

// Forward declarations
struct SpvRelocationStream;
struct SpvDebugMap;
struct SpvSourceMap;
struct SpvPhysicalBlockTable;

class SpvModule {
public:
    SpvModule(const Allocators& allocators, uint64_t shaderGUID);
    ~SpvModule();

    /// No copy
    SpvModule(const SpvModule& other) = delete;
    SpvModule& operator=(const SpvModule& other) = delete;

    /// No move
    SpvModule(SpvModule&& other) = delete;
    SpvModule& operator=(SpvModule&& other) = delete;

    /// Copy this module
    /// \return
    SpvModule* Copy() const;

    /// Parse a module
    /// \param code the SPIRV module pointer
    /// \param wordCount number of words within the module stream
    bool ParseModule(const uint32_t* code, uint32_t wordCount);

    /// Recompile the program, code must be the same as the originally parsed module
    /// \param code the SPIRV module pointer
    /// \param wordCount number of words within the module stream
    /// \return success state
    bool Recompile(const uint32_t* code, uint32_t wordCount, const SpvJob& job);

    /// Get the produced program
    /// \return
    const IL::Program* GetProgram() const {
        return program;
    }

    /// Get the produced program
    /// \return
    IL::Program* GetProgram() {
        return program;
    }

    /// Get the code pointer
    const uint32_t* GetCode() const {
        return spirvProgram.GetData();
    }

    /// Get the byte size of the code
    uint64_t GetSize() const {
        return spirvProgram.GetWordCount() * sizeof(uint32_t);
    }

    /// Get the source map for this module
    const SpvSourceMap* GetSourceMap() const;

    /// Get the parent module
    const SpvModule* GetParent() const {
        return parent;
    }

private:
    /// Parent instance
    const SpvModule* parent{nullptr};

private:
    Allocators allocators;

    /// Global GUID
    uint64_t shaderGUID{~0ull};

    /// JIT'ed program
    SpvStream spirvProgram;

    /// The physical block table, contains all spirv data
    SpvPhysicalBlockTable* physicalBlockTable{nullptr};

    /// Abstracted program
    IL::Program* program{nullptr};
};
