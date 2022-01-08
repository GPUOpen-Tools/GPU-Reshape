#include <Backends/Vulkan/Compiler/SpvModule.h>
#include <Backends/Vulkan/Compiler/SpvInstruction.h>
#include <Backends/Vulkan/Compiler/SpvStream.h>
#include <Backends/Vulkan/Compiler/SpvDebugMap.h>
#include "Backends/Vulkan/Compiler/SpvSourceMap.h"

bool SpvModule::ParseModule(const uint32_t *code, uint32_t wordCount) {
    program = new(allocators) IL::Program(allocators, shaderGUID);
    typeMap = new(allocators) SpvTypeMap(allocators, &program->GetTypeMap());
    debugMap = new(allocators) SpvDebugMap();
    sourceMap = new(allocators) SpvSourceMap();

    ParseContext context;
    context.start = code;
    context.code = code;
    context.end = code + wordCount;

    // Attempt to parse the header
    if (!ParseHeader(context)) {
        return false;
    }

    // Summarize all sections
    SummarizePhysicalSections(context);

    // Parse instruction stream
    while (context.Good()) {
        if (!ParseInstruction(context)) {
            return false;
        }
    }

    // Must have been terminated
    if (context.basicBlock) {
        return false;
    }

    // OK
    return true;
}

bool SpvModule::ParseHeader(ParseContext &context) {
    if (context.WordCount() < 5) {
        return false;
    }

    // Read the header
    header = context.Read<ProgramHeader>();

    // Check the magic number
    if (header.magic != SpvMagicNumber) {
        return false;
    }

    // Set the number of bound IDs for further allocation
    program->GetIdentifierMap().SetBound(header.bound);

    // Set the debug head
    debugMap->SetBound(header.bound);

    // OK
    return true;
}

bool SpvModule::ParseInstruction(ParseContext &context) {
    uint32_t sourceWord = context.Source();

    IL::Source source = IL::Source::Code(sourceWord);

    // Instruction pointer
    auto *instruction = reinterpret_cast<const SpvInstruction *>(context.code);

    // Number of words
    uint32_t sourceWordCount = instruction->GetWordCount();

    // End
    const uint32_t* end = context.code + sourceWordCount;

    // Eat header
    context.code++;

    // Instruction op code
    auto opCode = instruction->GetOp();

    // Must be able to accommodate
    if (end > context.end) {
        return false;
    }

    // Does this instruction have a result or result type?
    bool hasResult;
    bool hasResultType;
    SpvHasResultAndType(opCode, &hasResult, &hasResultType);

    // Parse type operand
    IL::ID idType = IL::InvalidID;
    if (hasResultType) {
        idType = context++;
    }

    // Parse id operand
    IL::ID id = IL::InvalidID;
    if (hasResult) {
        id = context++;

        // If there's an associated type, map it
        if (idType != IL::InvalidID) {
            const Backend::IL::Type* type = typeMap->GetTypeFromId(idType);
            if (!type) {
                // Map as unexposed
                type = typeMap->AddType(idType, Backend::IL::UnexposedType{});
            }

            // Create type -> id mapping
            program->GetTypeMap().SetType(id, type);
        }
    }

    // Handled op codes
    switch (opCode) {
        case SpvOpCapability: {
            auto capability = static_cast<SpvCapability>(context++);
            metaData.capabilities.insert(capability);
            break;
        }

        // Start of a function
        case SpvOpFunction: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Allocate a new function
            context.function = program->AllocFunction(id);
            context.basicBlock = nullptr;
            context.basicBlockSourceSpan = {};

            // Begins at the function instruction
            context.functionSourceSpan.begin = sourceWord;
            context.functionSourceSpan.end = 0;
            break;
        }

            // End of a function
        case SpvOpFunctionEnd: {
            // Terminate current basic block
            if (context.basicBlock) {
                // Set end source
                context.basicBlockSourceSpan.end = sourceWord;

                // Immortalize the current block
                context.basicBlock->Immortalize(context.basicBlockSourceSpan);
            }

            // Set end source (after SpvOpFunctionEnd)
            context.functionSourceSpan.end = sourceWord + sourceWordCount;

            // Terminate the current function
            context.function->Immortalize(context.functionSourceSpan);

            // Clean context
            context.function = nullptr;
            context.basicBlock = nullptr;
            context.basicBlockSourceSpan = {};
            context.functionSourceSpan = {};
            break;
        }

            // Label instruction, essentially operates as a tag for branches
        case SpvOpLabel: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Terminate current basic block
            if (context.basicBlock) {
                // Set end source
                context.basicBlockSourceSpan.end = sourceWord;

                // Immortalize the current block
                context.basicBlock->Immortalize(context.basicBlockSourceSpan);
            }

            // Allocate a new basic block
            context.basicBlock = context.function->AllocBlock(id);

            // Begins at the label
            context.basicBlockSourceSpan.begin = sourceWord;
            context.basicBlockSourceSpan.end = 0;
            break;
        }

        // Metadata decoration
        case SpvOpDecorate: {
            uint32_t target = context++;
            auto decoration = static_cast<SpvDecoration>(context++);

            // Handle decoration
            switch (decoration) {
                default: {
                    break;
                }
                case SpvDecorationDescriptorSet: {
                    metaData.boundDescriptorSets = std::max(metaData.boundDescriptorSets, context++);
                    break;
                }
            }
            break;
        }

        // Debug

        case SpvOpString: {
            ASSERT(hasResult, "Expected result for instruction type");
            debugMap->Add(id, SpvOpString, std::string_view(reinterpret_cast<const char*>(context.code), (sourceWordCount - 2) * sizeof(uint32_t)));
            break;
        }

        case SpvOpSource: {
            auto language = static_cast<SpvSourceLanguage>(context++);
            uint32_t version = context++;

            // Optional filename
            uint32_t fileId{InvalidSpvId};
            if (context.code < end) {
                fileId = context++;
                sourceMap->AddPhysicalSource(fileId, language, version, debugMap->Get(fileId, SpvOpString));
            }

            // Optional fragment?
            if (context.code < end) {
                sourceMap->AddSource(fileId, std::string_view(reinterpret_cast<const char*>(context.code), (sourceWordCount - 4) * sizeof(uint32_t)));
            }
            break;
        }

        case SpvOpSourceContinued: {
            sourceMap->AddSource(sourceMap->GetPendingSource(), std::string_view(reinterpret_cast<const char*>(context.code), (sourceWordCount - 1) * sizeof(uint32_t)));
            break;
        }

        case SpvOpLine: {
            if (!context.basicBlock) {
                break;
            }

            // Append
            IL::SourceAssociationInstruction instr{};
            instr.opCode = IL::OpCode::SourceAssociation;
            instr.result = IL::InvalidID;
            instr.source = source;
            instr.file = sourceMap->GetFileIndex(context++);
            instr.line = (context++) - 1;
            instr.column = (context++) - 1;
            context.basicBlock->Append(instr);
            break;
        }

        // Integer type
        case SpvOpTypeInt: {
            ASSERT(hasResult, "Expected result for instruction type");

            Backend::IL::IntType type;
            type.bitWidth = context++;
            type.signedness = context++;
            typeMap->AddType(id, type);
            break;
        }

        case SpvOpTypeVoid: {
            ASSERT(hasResult, "Expected result for instruction type");

            Backend::IL::VoidType type;
            typeMap->AddType(id, type);
            break;
        }

        case SpvOpTypeBool: {
            ASSERT(hasResult, "Expected result for instruction type");

            Backend::IL::BoolType type;
            typeMap->AddType(id, type);
            break;
        }

        case SpvOpTypeFloat: {
            ASSERT(hasResult, "Expected result for instruction type");

            Backend::IL::FPType type;
            type.bitWidth = context++;
            typeMap->AddType(id, type);
            break;
        }

        case SpvOpTypeVector: {
            ASSERT(hasResult, "Expected result for instruction type");

            Backend::IL::VectorType type;
            type.containedType = typeMap->GetTypeFromId(context++);
            type.dimension = context++;
            typeMap->AddType(id, type);
            break;
        }

        case SpvOpTypeMatrix: {
            ASSERT(hasResult, "Expected result for instruction type");

            const Backend::IL::Type* columnType = typeMap->GetTypeFromId(context++);

            auto* columnVector = columnType->Cast<Backend::IL::VectorType>();
            ASSERT(columnVector, "Column type must be vector");

            Backend::IL::MatrixType type;
            type.containedType = columnVector->containedType;
            type.rows = columnVector->dimension;
            type.columns = context++;
            typeMap->AddType(id, type);
            break;
        }

        case SpvOpTypePointer: {
            ASSERT(hasResult, "Expected result for instruction type");

            Backend::IL::PointerType type;
            type.addressSpace = Translate(static_cast<SpvStorageClass>(context++));
            type.pointee = typeMap->GetTypeFromId(context++);
            typeMap->AddType(id, type);
            break;
        }

        case SpvOpTypeArray: {
            ASSERT(hasResult, "Expected result for instruction type");

            Backend::IL::ArrayType type;
            type.elementType = typeMap->GetTypeFromId(context++);
            type.count = context++;

            typeMap->AddType(id, type);
            break;
        }

        case SpvOpTypeImage: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Sampled type
            const Backend::IL::Type* sampledType = typeMap->GetTypeFromId(context++);

            // Dimension of the image
            SpvDim dim = static_cast<SpvDim>(context++);

            // Cap operands
            bool isDepth = context++;
            bool isArrayed = context++;
            bool multisampled = context++;
            bool requiresSampler = context++;

            // Format, if present
            Backend::IL::Format format = Translate(static_cast<SpvImageFormat>(context++));

            // Texel buffer?
            if (dim == SpvDimBuffer) {
                Backend::IL::BufferType type;
                type.elementType = sampledType;
                type.texelType = format;
                typeMap->AddType(id, type);
            } else {
                Backend::IL::TextureType type;
                type.sampledType = sampledType;
                type.dimension = Translate(dim);

                if (isArrayed) {
                    switch (type.dimension) {
                        default:
                            type.dimension = Backend::IL::TextureDimension::Unexposed;
                            break;
                        case Backend::IL::TextureDimension::Texture1D:
                            type.dimension = Backend::IL::TextureDimension::Texture1DArray;
                            break;
                        case Backend::IL::TextureDimension::Texture2D:
                            type.dimension = Backend::IL::TextureDimension::Texture2DArray;
                            break;
                    }
                }

                type.multisampled = multisampled;
                type.requiresSampler = requiresSampler;
                type.format = format;

                typeMap->AddType(id, type);
            }
            break;
        }

        case SpvOpLoad: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::LoadInstruction instr{};
            instr.opCode = IL::OpCode::Load;
            instr.result = id;
            instr.source = source;
            instr.address = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpStore: {
            // Append
            IL::StoreInstruction instr{};
            instr.opCode = IL::OpCode::Store;
            instr.result = id;
            instr.source = source;
            instr.address = context++;
            instr.value = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpFAdd:
        case SpvOpIAdd: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::AddInstruction instr{};
            instr.opCode = IL::OpCode::Add;
            instr.result = id;
            instr.source = source;
            instr.lhs = context++;
            instr.rhs = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpFSub:
        case SpvOpISub: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::SubInstruction instr{};
            instr.opCode = IL::OpCode::Sub;
            instr.result = id;
            instr.source = source;
            instr.lhs = context++;
            instr.rhs = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpFDiv:
        case SpvOpSDiv:
        case SpvOpUDiv: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::DivInstruction instr{};
            instr.opCode = IL::OpCode::Div;
            instr.result = id;
            instr.source = source;
            instr.lhs = context++;
            instr.rhs = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpFMul:
        case SpvOpIMul: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::MulInstruction instr{};
            instr.opCode = IL::OpCode::Mul;
            instr.result = id;
            instr.source = source;
            instr.lhs = context++;
            instr.rhs = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpLogicalAnd: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::AndInstruction instr{};
            instr.opCode = IL::OpCode::And;
            instr.result = id;
            instr.source = source;
            instr.lhs = context++;
            instr.rhs = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpLogicalOr: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::OrInstruction instr{};
            instr.opCode = IL::OpCode::Or;
            instr.result = id;
            instr.source = source;
            instr.lhs = context++;
            instr.rhs = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpLogicalEqual:
        case SpvOpIEqual:
        case SpvOpFOrdEqual:
        case SpvOpFUnordEqual: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::EqualInstruction instr{};
            instr.opCode = IL::OpCode::Equal;
            instr.result = id;
            instr.source = source;
            instr.lhs = context++;
            instr.rhs = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpLogicalNotEqual:
        case SpvOpINotEqual:
        case SpvOpFOrdNotEqual:
        case SpvOpFUnordNotEqual: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::EqualInstruction instr{};
            instr.opCode = IL::OpCode::NotEqual;
            instr.result = id;
            instr.source = source;
            instr.lhs = context++;
            instr.rhs = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpSLessThan:
        case SpvOpULessThan:
        case SpvOpFOrdLessThan:
        case SpvOpFUnordLessThan: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::LessThanInstruction instr{};
            instr.opCode = IL::OpCode::LessThan;
            instr.result = id;
            instr.source = source;
            instr.lhs = context++;
            instr.rhs = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpSLessThanEqual:
        case SpvOpULessThanEqual:
        case SpvOpFOrdLessThanEqual:
        case SpvOpFUnordLessThanEqual: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::LessThanInstruction instr{};
            instr.opCode = IL::OpCode::LessThanEqual;
            instr.result = id;
            instr.source = source;
            instr.lhs = context++;
            instr.rhs = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpSGreaterThan:
        case SpvOpUGreaterThan:
        case SpvOpFOrdGreaterThan:
        case SpvOpFUnordGreaterThan: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::GreaterThanInstruction instr{};
            instr.opCode = IL::OpCode::GreaterThan;
            instr.result = id;
            instr.source = source;
            instr.lhs = context++;
            instr.rhs = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpSGreaterThanEqual:
        case SpvOpUGreaterThanEqual:
        case SpvOpFOrdGreaterThanEqual:
        case SpvOpFUnordGreaterThanEqual: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::GreaterThanInstruction instr{};
            instr.opCode = IL::OpCode::GreaterThanEqual;
            instr.result = id;
            instr.source = source;
            instr.lhs = context++;
            instr.rhs = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpBitwiseOr: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::BitOrInstruction instr{};
            instr.opCode = IL::OpCode::BitOr;
            instr.result = id;
            instr.source = source;
            instr.lhs = context++;
            instr.rhs = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpBitwiseAnd: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::BitOrInstruction instr{};
            instr.opCode = IL::OpCode::BitAnd;
            instr.result = id;
            instr.source = source;
            instr.lhs = context++;
            instr.rhs = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpShiftLeftLogical: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::BitShiftLeftInstruction instr{};
            instr.opCode = IL::OpCode::BitShiftLeft;
            instr.result = id;
            instr.source = source;
            instr.value = context++;
            instr.shift = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpShiftRightLogical:
        case SpvOpShiftRightArithmetic: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::BitShiftLeftInstruction instr{};
            instr.opCode = IL::OpCode::BitShiftRight;
            instr.result = id;
            instr.source = source;
            instr.value = context++;
            instr.shift = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpBranch: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::BranchInstruction instr{};
            instr.opCode = IL::OpCode::Branch;
            instr.result = id;
            instr.source = source;
            instr.branch = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpBranchConditional: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Append
            IL::BranchConditionalInstruction instr{};
            instr.opCode = IL::OpCode::BranchConditional;
            instr.result = id;
            instr.source = source;
            instr.cond = context++;
            instr.pass = context++;
            instr.fail = context++;
            context.basicBlock->Append(instr);
            break;
        }

        case SpvOpVariable: {
            ASSERT(hasResult, "Expected result for instruction type");
            ASSERT(hasResultType, "Expected result type for instruction type");

            const Backend::IL::Type* type = typeMap->GetTypeFromId(idType);
            program->GetTypeMap().SetType(id, type);

            // Append
            IL::AllocaInstruction instr{};
            instr.opCode = IL::OpCode::Alloca;
            instr.result = id;
            instr.source = source;
            instr.type = idType;
            // context.basicBlock->Append(instr);
            break;
        }

            // Integral literal
        case SpvOpConstant: {
            ASSERT(hasResult, "Expected result for instruction type");

            uint32_t type = context++;
            uint32_t value = context++;

            // Append
            IL::LiteralInstruction instr{};
            instr.opCode = IL::OpCode::Literal;
            instr.result = id;
            instr.source = source;
            instr.type = IL::LiteralType::Int;
            instr.signedness = true;
            instr.bitWidth = 32;
            instr.value.integral = value;
            context.basicBlock->Append(instr);
            break;
        }

            // Image store operation, fx. texture & buffer writes
        case SpvOpImageWrite: {
            uint32_t image = context++;
            uint32_t coordinate = context++;
            uint32_t texel = context++;

            // Append
            IL::StoreBufferInstruction instr{};
            instr.opCode = IL::OpCode::StoreBuffer;
            instr.result = id;
            instr.source = source;
            instr.buffer = image;
            instr.index = coordinate;
            instr.value = texel;
            context.basicBlock->Append(instr);
            break;
        }

            // Unexposed
        default: {
            // May not have a target basic block, such as program metadata
            if (context.basicBlock) {
                // Emit as unexposed
                IL::UnexposedInstruction instr{};
                instr.opCode = IL::OpCode::Unexposed;
                instr.result = id;
                instr.source = source;
                instr.backendOpCode = opCode;
                context.basicBlock->Append(instr);
            }
            break;
        }
    }

    // Set to end
    ASSERT(context.code <= end, "Read beyond the instruction word count");
    context.code = end;

    // OK
    return true;
}

const SpvModule::LayoutSection &SpvModule::GetSection(LayoutSectionType type) {
    return metaData.sections[static_cast<uint32_t>(type)];
}

bool SpvModule::SummarizePhysicalSections(ParseContext context) {
    uint32_t sectionBegin = context.Source();

    // Init sections
    for (LayoutSection& section : metaData.sections) {
        section.sourceSpan.begin = IL::InvalidOffset;
        section.sourceSpan.end = 0u;
    }

    // Parse instruction stream
    while (context.Good()) {
        // Instruction pointer
        auto *instruction = reinterpret_cast<const SpvInstruction *>(context.code);

        // Instruction span
        IL::SourceSpan span = {context.Source(), context.Source() + instruction->GetWordCount()};

        // Operation code
        SpvOp op = instruction->GetOp();

        bool isFunction{false};

        // Handled op codes
        LayoutSectionType type{LayoutSectionType::Count};
        switch (op) {
            default:
                // Type, constant and other semantic operations, are filled out during patching
                type = LayoutSectionType::Declarations;
                break;

            case SpvOpCapability:
                type = LayoutSectionType::Capability;
                break;
            case SpvOpExtension:
                type = LayoutSectionType::Extension;
                break;

            case SpvOpExtInstImport:
                type = LayoutSectionType::InstImport;
                break;

            case SpvOpMemoryModel:
                type = LayoutSectionType::MemoryModel;
                break;

            case SpvOpEntryPoint:
                type = LayoutSectionType::Entry;
                break;

            case SpvOpExecutionMode:
            case SpvOpExecutionModeId:
                type = LayoutSectionType::Execution;
                break;

            case SpvOpString:
            case SpvOpSourceExtension:
            case SpvOpSource:
            case SpvOpSourceContinued:
                type = LayoutSectionType::DebugString;
                break;

            case SpvOpName:
            case SpvOpMemberName:
                type = LayoutSectionType::DebugName;
                break;

            case SpvOpModuleProcessed:
                type = LayoutSectionType::DebugModule;
                break;

            case SpvOpDecorate:
            case SpvOpMemberDecorate:
            case SpvOpDecorationGroup:
            case SpvOpGroupDecorate:
            case SpvOpGroupMemberDecorate:
            case SpvOpDecorateId:
            case SpvOpDecorateString:
            case SpvOpMemberDecorateString:
                type = LayoutSectionType::Annotation;
                break;

            case SpvOpFunction:
            case SpvOpFunctionEnd:
                isFunction = true;
                break;
        }

        if (isFunction) {
            break;
        }

        // Relevant?
        if (type != LayoutSectionType::Count) {
            LayoutSection& section = metaData.sections[static_cast<uint32_t>(type)];
            section.sourceSpan.begin = std::min(section.sourceSpan.begin, span.begin);
            section.sourceSpan.end = std::max(section.sourceSpan.end, span.end);
        }

        // Next
        context.code += instruction->GetWordCount();
    }

    // Fill all missing sections
    for (auto i = 0; i < static_cast<uint32_t>(LayoutSectionType::Count); i++) {
        LayoutSection& section = metaData.sections[i];

        // Skip valid sections
        if (section.sourceSpan.begin != IL::InvalidOffset)
            continue;

        // May be missing capabilities
        if (i == 0) {
            // Start at base
            section.sourceSpan = {sectionBegin, sectionBegin};
        } else {
            // Start after the previous
            LayoutSection& previous = metaData.sections[i - 1];
            section.sourceSpan = {previous.sourceSpan.end, previous.sourceSpan.end};
        }
    }

    return true;
}
