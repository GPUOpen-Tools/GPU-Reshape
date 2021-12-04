#include <Backends/Vulkan/Compiler/SpvModule.h>
#include <Backends/Vulkan/Compiler/SpvInstruction.h>
#include <Backends/Vulkan/Compiler/SpvStream.h>
#include <Backends/Vulkan/Compiler/SpvRelocationStream.h>

bool SpvModule::ParseModule(const uint32_t *code, uint32_t wordCount) {
    program = new(allocators) IL::Program(allocators);
    typeMap = new(allocators) SpvTypeMap(allocators);

    ParseContext context;
    context.start = code;
    context.code = code;
    context.end = code + wordCount;

    // Attempt to parse the header
    if (!ParseHeader(context)) {
        return false;
    }

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

    // Set the type head
    typeMap->SetIdBound(header.bound);

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
            const SpvType* type = typeMap->GetType(idType);
            ASSERT(type != nullptr, "Unexposed types not implemented");

            // Create type -> id mapping
            typeMap->SetType(id, type);
        }
    }

    // Handled op codes
    switch (opCode) {
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

            // Integer type
        case SpvOpTypeInt: {
            ASSERT(hasResult, "Expected result for instruction type");

            SpvIntType type;
            type.id = id;
            type.bitWidth = context++;
            type.signedness = context++;
            typeMap->AddType(type);
            break;
        }

        case SpvOpTypeVoid: {
            ASSERT(hasResult, "Expected result for instruction type");

            SpvVoidType type;
            type.id = id;
            typeMap->AddType(type);
            break;
        }

        case SpvOpTypeBool: {
            ASSERT(hasResult, "Expected result for instruction type");

            SpvBoolType type;
            type.id = id;
            typeMap->AddType(type);
            break;
        }

        case SpvOpTypeFloat: {
            ASSERT(hasResult, "Expected result for instruction type");

            SpvFPType type;
            type.id = id;
            type.bitWidth = context++;
            typeMap->AddType(type);
            break;
        }

        case SpvOpTypeVector: {
            ASSERT(hasResult, "Expected result for instruction type");

            SpvVectorType type;
            type.id = id;
            type.containedType = typeMap->GetType(context++);
            type.dimension = context++;
            typeMap->AddType(type);
            break;
        }

        case SpvOpTypeMatrix: {
            ASSERT(hasResult, "Expected result for instruction type");

            const SpvType* columnType = typeMap->GetType(context++);

            auto* columnVector = columnType->Cast<SpvVectorType>();
            ASSERT(columnVector, "Column type must be vector");

            SpvMatrixType type;
            type.id = id;
            type.containedType = columnVector->containedType;
            type.rows = columnVector->dimension;
            type.columns = context++;
            typeMap->AddType(type);
            break;
        }

        case SpvOpTypePointer: {
            ASSERT(hasResult, "Expected result for instruction type");

            // Eat storage
            context++;

            const SpvType* pointee = typeMap->GetType(context++);

            SpvPointerType type;
            type.id = id;
            type.pointee = pointee;
            typeMap->AddType(type);
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

            const SpvType* type = typeMap->GetType(idType);
            typeMap->SetType(id, type);

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
