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
        return spirvProgram.data();
    }

    /// Get the byte size of the code
    uint64_t GetSize() const {
        return spirvProgram.size() * sizeof(uint32_t);
    }

    /// Get the source map for this module
    const SpvSourceMap* GetSourceMap() const {
        return sourceMap;
    }

    /// Get the debug map for this module
    const SpvDebugMap* GetDebugMap() const {
        return debugMap;
    }

private:
    /// Parsing context
    struct ParseContext {
        template<typename T>
        const T& Read() {
            auto* ptr = reinterpret_cast<const T*>(code);
            code += sizeof(T) / sizeof(uint32_t);
            return *ptr;
        }

        uint32_t operator*() const {
            ASSERT(code < end, "End of stream");
            return *code;
        }

        uint32_t operator++() {
            ASSERT(code < end, "End of stream");
            return *(++code);
        }

        uint32_t operator++(int) {
            ASSERT(code < end, "End of stream");
            return *(code++);
        }

        uint32_t WordCount() const {
            return end - code;
        }

        uint32_t Good() const {
            return end > code;
        }

        uint32_t Source() const {
            return code - start;
        }

        /// Code pointers
        const uint32_t* start;
        const uint32_t* code;
        const uint32_t* end;

        /// Current function
        IL::Function* function{nullptr};

        /// Current basic block
        IL::BasicBlock* basicBlock{nullptr};

        /// Current source spans
        IL::SourceSpan functionSourceSpan{};
        IL::SourceSpan basicBlockSourceSpan{};
    };

    /// Parse the header
    /// \param context
    /// \return
    bool ParseHeader(ParseContext& context);

    /// Summarize all physical sections
    /// \param context parsing context, copied
    /// \return success state
    bool SummarizePhysicalSections(ParseContext context);

    /// Parse an instruction
    /// \param context
    /// \return
    bool ParseInstruction(ParseContext& context);

private:
    /// Header specification
    struct ProgramHeader {
        uint32_t magic;
        uint32_t version;
        uint32_t generator;
        uint32_t bound;
        uint32_t reserved;
    };

    /// Sectiont ype
    enum class LayoutSectionType {
        Capability,
        Extension,
        InstImport,
        MemoryModel,
        Entry,
        Execution,
        DebugString,
        DebugName,
        DebugModule,
        Annotation,
        Declarations,
        FunctionDeclaration,
        FunctionDefinition,
        Count
    };

    /// Single section
    struct LayoutSection {
        IL::SourceSpan sourceSpan;
    };

    /// All export metadata
    struct ExportMetaData {
        /// Spv identifiers
        uint32_t counterId{0};
        uint32_t streamId{0};

        /// Type map
        const SpvType* image32UIRWArrayPtr;
        const SpvType* image32UIRWPtr;
        const SpvType* image32UIRW;
    };

    /// Program metadata
    struct MetaData {
        /// Number of bound descriptor sets
        uint32_t boundDescriptorSets{0};

        /// All capabilities
        std::set<SpvCapability> capabilities;

        /// All physical sections
        LayoutSection sections[static_cast<uint32_t>(LayoutSectionType::Count)]{};

        /// Export metadata
        ExportMetaData exportMd;
    };

    /// Header
    ProgramHeader header;

    /// Copyable meta data
    MetaData metaData;

    /// Debug map
    SpvDebugMap* debugMap{nullptr};

    /// Source map
    SpvSourceMap* sourceMap{nullptr};

    /// Parent instance
    const SpvModule* parent{nullptr};

private:
    /// Get a single section
    /// \param type type of the section
    const LayoutSection& GetSection(LayoutSectionType type);

    /// Insert the export records
    /// \param jitHeader the jitted program header
    /// \param stream the final relocation stream
    /// \param job current job
    void InsertExportRecords(ProgramHeader& jitHeader, SpvRelocationStream &stream, const SpvJob& job);

private:
    Allocators allocators;

    /// Global GUID
    uint64_t shaderGUID{~0ull};

    /// JIT'ed program
    std::vector<uint32_t> spirvProgram;

    /// The type map
    SpvTypeMap* typeMap{nullptr};

    /// Abstracted program
    IL::Program* program{nullptr};
};
