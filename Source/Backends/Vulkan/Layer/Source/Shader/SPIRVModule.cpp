#include <Backends/Vulkan/Shader/SPIRVModule.h>

// SPIRV
#define SPV_ENABLE_UTILITY_CODE
#include <spirv/unified1/spirv.h>

bool SPIRVModule::ParseModule(const uint32_t *code, uint32_t wordCount) {
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
    program.SetBound(header.bound);

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
            context.function = program.AllocFunction(id);
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

bool SPIRVModule::Recompile() {
    return true;
}
