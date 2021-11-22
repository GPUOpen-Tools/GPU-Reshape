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

    // OK
    return true;
}

bool SpvModule::ParseInstruction(ParseContext &context) {
    uint32_t source = context.Source();

    auto *instruction = reinterpret_cast<const SpvInstruction *>(context.code);
    context.code++;

    // Remaining words
    uint32_t sourceWords = instruction->GetWordCount();
    uint32_t remainingWords = sourceWords - 1;

    // Instruction op code
    auto opCode = instruction->GetOp();

    // Must be able to accommodate
    if (context.WordCount() < remainingWords) {
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
        remainingWords--;
    }

    // Parse id operand
    IL::ID id = IL::InvalidID;
    if (hasResult) {
        id = context++;
        remainingWords--;
    }

    // Handled op codes
    switch (opCode) {
        // Start of a function
        case SpvOpFunction: {
            if (!hasResult) {
                return false;
            }

            // Allocate a new function
            context.function = program->AllocFunction(id);
            context.basicBlock = nullptr;
            context.basicBlockSourceSpan = {};

            // Begins at the function instruction
            context.functionSourceSpan.begin = source;
            context.functionSourceSpan.end = 0;
            break;
        }

            // End of a function
        case SpvOpFunctionEnd: {
            // Terminate current basic block
            if (context.basicBlock) {
                // Set end source
                context.basicBlockSourceSpan.end = source;

                // Immortalize the current block
                context.basicBlock->Immortalize(context.basicBlockSourceSpan);
            }

            // Set end source (after SpvOpFunctionEnd)
            context.functionSourceSpan.end = source + sourceWords;

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
            if (!hasResult) {
                return false;
            }

            // Terminate current basic block
            if (context.basicBlock) {
                // Set end source
                context.basicBlockSourceSpan.end = source;

                // Immortalize the current block
                context.basicBlock->Immortalize(context.basicBlockSourceSpan);
            }

            // Allocate a new basic block
            context.basicBlock = context.function->AllocBlock(id);

            // Begins at the first instruction inside the basic block
            context.basicBlockSourceSpan.begin = source + sourceWords;
            context.basicBlockSourceSpan.end = 0;
            break;
        }

            // Integer type
        case SpvOpTypeInt: {
            if (!hasResult) {
                return false;
            }

            SpvIntType type;
            type.id = id;
            type.bitWidth = context++;
            type.signedness = context++;
            typeMap->AddType(type);

            remainingWords -= 2;
            break;
        }

            // Integral literal
        case SpvOpConstant: {
            if (!hasResult) {
                return false;
            }

            uint32_t type = context++;
            uint32_t value = context++;
            remainingWords -= 2;

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
            remainingWords -= 3;

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

    // Consume ignored operands
    for (; remainingWords; --remainingWords) {
        context++;
    }

    // OK
    return true;
}

bool SpvModule::Recompile(const uint32_t *code, uint32_t wordCount) {
    // Compilation relocation stream
    SpvRelocationStream relocationStream(code, wordCount);

    // Get the header
    ProgramHeader jitHeader = header;
    relocationStream.AllocateFixedBlock(IL::SourceSpan{0, IL::WordCount<ProgramHeader>()}, &jitHeader);

    // Set the new number of bound values
    jitHeader.bound = program->GetIdentifierMap().GetMaxID();

    // Set the type counter
    typeMap->SetIdCounter(&jitHeader.bound);

    // Go through all functions
    for (auto fn = program->begin(); fn != program->end(); fn++) {
        // Create the declaration relocation block
        SpvRelocationBlock &declarationRelocationBlock = relocationStream.AllocateBlock(fn->GetDeclarationSourceSpan());
        typeMap->SetDeclarationStream(&declarationRelocationBlock.stream);

        // Find all dirty basic blocks
        for (auto bb = fn->rbegin(); bb != fn->rend(); bb++) {
            // If not modified, skip
            if (!bb->IsModified()) {
                continue;
            }

            // Create a new relocation block
            SpvRelocationBlock &relocationBlock = relocationStream.AllocateBlock(bb->GetSourceSpan());

            // Destination stream
            SpvStream &stream = relocationBlock.stream;

            // Emit all backend instructions
            for (auto instr = bb->begin(); instr != bb->end(); instr++) {
                switch (instr->opCode) {
                    default: {
                        ASSERT(false, "Invalid instruction in basic block");
                        return false;
                    }
                    case IL::OpCode::Unexposed: {
                        stream.Template(instr->source);
                        break;
                    }
                    case IL::OpCode::Literal: {
                        auto *literal = instr.As<IL::LiteralInstruction>();

                        SpvIntType typeInt;
                        typeInt.bitWidth = literal->bitWidth;
                        typeInt.signedness = literal->signedness;
                        SpvId typeIntId = typeMap->GetIdOrAdd(typeInt);

                        auto *constant = declarationRelocationBlock.stream.Allocate<SpvConstantInstruction>();
                        constant->result = literal->result;
                        constant->resultType = typeIntId;
                        constant->value = literal->value.integral;
                        break;
                    }
                    case IL::OpCode::Add: {
                        auto *add = instr.As<IL::AddInstruction>();

                        SpvIntType typeInt;
                        typeInt.bitWidth = 32;
                        typeInt.signedness = 0;
                        SpvId typeIntId = typeMap->GetIdOrAdd(typeInt);

                        auto *constant = stream.Allocate<SpvIAddInstruction>();
                        constant->result = add->result;
                        constant->resultType = typeIntId;
                        constant->lhs = add->lhs;
                        constant->rhs = add->rhs;
                        break;
                    }
                    case IL::OpCode::LoadTexture:
                    case IL::OpCode::Store:
                    case IL::OpCode::Load: {
                        ASSERT(false, "Not implemented");
                        break;
                    }
                    case IL::OpCode::StoreBuffer: {
                        auto *storeBuffer = instr.As<IL::StoreBufferInstruction>();

                        // Write image
                        auto *spv = stream.TemplateOrAllocate<SpvImageWritePartialInstruction>(instr->source);
                        spv->image = storeBuffer->buffer;
                        spv->coordinate = storeBuffer->index;
                        spv->texel = storeBuffer->value;
                        break;
                    }
                }
            }
        }
    }

    // Stitch the final program
    spirvProgram.clear();
    relocationStream.Stitch(spirvProgram);

    // OK!
    return true;
}
