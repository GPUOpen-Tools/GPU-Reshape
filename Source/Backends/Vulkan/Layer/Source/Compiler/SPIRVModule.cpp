#include <Backends/Vulkan/Compiler/SPIRVModule.h>

// SPIRV
#define SPV_ENABLE_UTILITY_CODE
#include <spirv/unified1/spirv.h>

SPIRVModule::SPIRVModule(const Allocators& allocators) : allocators(allocators) {
}

SPIRVModule::~SPIRVModule() {
    destroy(program, allocators);
}

SPIRVModule *SPIRVModule::Copy() const {
    auto* module = new (allocators) SPIRVModule(allocators);
    module->header = header;
    module->spirvProgram = spirvProgram;
    module->program = program->Copy();
    return module;
}

bool SPIRVModule::ParseModule(const uint32_t *code, uint32_t wordCount) {
    program = new (allocators) IL::Program(allocators);

    ParseContext context;
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
    if (context.emitter.Good()) {
        return false;
    }

    // OK
    return true;
}

bool SPIRVModule::ParseHeader(ParseContext &context) {
    if (context.WordCount() < 5) {
        return false;
    }

    // Check the magic number
    if (context++ != SpvMagicNumber) {
        return false;
    }

    // Parse the header
    header.version = context++;
    header.generator = context++;
    header.bound = context++;
    header.reserved = context++;

    // Set the number of bound IDs for further allocation
    program->GetIdentifierMap().SetBound(header.bound);

    // OK
    return true;
}

bool SPIRVModule::ParseInstruction(ParseContext &context) {
    uint32_t lowWordCountHighOpCode = context++;

    // Remaining words
    uint32_t remainingWords = ((lowWordCountHighOpCode >> SpvWordCountShift) & SpvOpCodeMask) - 1;

    // Instruction op code
    auto opCode = static_cast<SpvOp>(lowWordCountHighOpCode & SpvOpCodeMask);

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
            context.emitter.SetBasicBlock(nullptr);
            break;
        }

        // End of a function
        case SpvOpFunctionEnd: {
            // Terminate current basic block
            if (IL::BasicBlock* basicBlock = context.emitter.GetBasicBlock()) {
                // Immortalize the current block
                basicBlock->Immortalize();
            }

            // Clean context
            context.function = nullptr;
            context.emitter.SetBasicBlock(nullptr);
            break;
        }

        // Label instruction, essentially operates as a tag for branches
        case SpvOpLabel: {
            if (!hasResult) {
                return false;
            }

            // Terminate current basic block
            if (IL::BasicBlock* basicBlock = context.emitter.GetBasicBlock()) {
                // Immortalize the current block
                basicBlock->Immortalize();
            }

            // Allocate a new basic block
            context.emitter.SetBasicBlock(context.function->AllocBlock(id));
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

            // Store buffer
            context.emitter.Integral(id, 32, value);
        }

        // Image store operation, fx. texture & buffer writes
        case SpvOpImageWrite: {
            uint32_t image = context++;
            uint32_t coordinate = context++;
            uint32_t texel = context++;
            remainingWords -= 3;

            // Store buffer
            context.emitter.StoreBuffer(image, coordinate, texel);
        }

        // Unexposed
        default: {
            // May not have a target basic block, such as program metadata
            if (context.emitter.Good()) {
                // Emit as unexposed
                context.emitter.Unexposed(id, opCode);
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

bool SPIRVModule::Recompile(const uint32_t* code, uint32_t wordCount) {
    /// Not done, as you may have deduced
    spirvProgram.insert(spirvProgram.end(), code, code + wordCount);
    return true;
}
