#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockFunction.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordReader.h>
#include <Backends/DX12/Compiler/DXIL/DXIL.Gen.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 */

void DXILPhysicalBlockFunction::Parse() {
    LLVMBlock &block = table.scan.GetRoot();

    // Visit all function blocks
    for (const LLVMBlock *childBlock: block.blocks) {
        if (!childBlock->Is(LLVMReservedBlock::Function)) {
            continue;
        }

        // Create function
        IL::Function* fn = program.GetFunctionList().AllocFunction();

        // Visit child blocks
        for (const LLVMBlock* fnBlock : childBlock->blocks) {
            switch (static_cast<LLVMReservedBlock>(fnBlock->id)) {
                default: {
                    break;
                }
                case LLVMReservedBlock::Constants: {
                    ParseConstants(fnBlock);
                    break;
                }
            }
        }

        // Visit function records
        ParseBlock(fn, childBlock);
    }
}

void DXILPhysicalBlockFunction::ParseBlock(IL::Function* fn, const struct LLVMBlock *block) {
    // Allocate basic block
    IL::BasicBlock* basicBlock = fn->GetBasicBlocks().AllocBlock();

    // Visit function records
    for (const LLVMRecord &record: block->records) {
        LLVMRecordReader reader(record);

        // Get value of this call
        /*uint64_t value = reader.ConsumeOp();

        // Optional type
        uint64_t type = UINT64_MAX;

        // Stores type?
        if (value >= record.id) {
            type = reader.ConsumeOp();
        }*/

        switch (static_cast<LLVMFunctionRecord>(record.id)) {
            default: {
                // Emit as unexposed
                IL::UnexposedInstruction instr{};
                instr.opCode = IL::OpCode::Unexposed;
                instr.result = 0x0;
                instr.source = IL::Source::Invalid();
                instr.backendOpCode = record.id;
                basicBlock->Append(instr);
                break;
            }
            /*case LLVMFunctionRecord::DeclareBlocks:
                break;
            case LLVMFunctionRecord::InstBinOp:
                break;
            case LLVMFunctionRecord::InstCast:
                break;
            case LLVMFunctionRecord::InstGEP:
                break;
            case LLVMFunctionRecord::InstSelect:
                break;
            case LLVMFunctionRecord::InstExtractELT:
                break;
            case LLVMFunctionRecord::InstInsertELT:
                break;
            case LLVMFunctionRecord::InstShuffleVec:
                break;
            case LLVMFunctionRecord::InstCmp:
                break;
            case LLVMFunctionRecord::InstRet:
                break;
            case LLVMFunctionRecord::InstBr:
                break;
            case LLVMFunctionRecord::InstSwitch:
                break;
            case LLVMFunctionRecord::InstInvoke:
                break;
            case LLVMFunctionRecord::InstUnwind:
                break;
            case LLVMFunctionRecord::InstUnreachable:
                break;
            case LLVMFunctionRecord::InstPhi:
                break;
            case LLVMFunctionRecord::InstMalloc:
                break;
            case LLVMFunctionRecord::InstFree:
                break;
            case LLVMFunctionRecord::InstAlloca:
                break;*/
            case LLVMFunctionRecord::InstLoad: {
                // Append
                IL::LoadInstruction instr{};
                instr.opCode = IL::OpCode::Load;
                instr.result = IL::InvalidID;
                instr.source = IL::Source::Invalid();
                instr.address = 0x0;
                basicBlock->Append(instr);
                break;
            }
            /*case LLVMFunctionRecord::InstStore:
                break;
            case LLVMFunctionRecord::InstCall:
                break;
            case LLVMFunctionRecord::InstVaArg:
                break;
            case LLVMFunctionRecord::InstStore2:
                break;
            case LLVMFunctionRecord::InstGetResult:
                break;
            case LLVMFunctionRecord::InstExtractVal:
                break;
            case LLVMFunctionRecord::InstInsertVal:
                break;
            case LLVMFunctionRecord::InstCmp2:
                break;
            case LLVMFunctionRecord::InstVSelect:
                break;
            case LLVMFunctionRecord::InstInBoundsGEP:
                break;
            case LLVMFunctionRecord::InstIndirectBR:
                break;
            case LLVMFunctionRecord::DebugLOC:
                break;
            case LLVMFunctionRecord::DebugLOCAgain:
                break;
            case LLVMFunctionRecord::InstCall2:
                break;
            case LLVMFunctionRecord::DebugLOC2:
                break;*/
        }
    }
}

void DXILPhysicalBlockFunction::ParseConstants(const LLVMBlock* block) {
    for (const LLVMRecord &record: block->records) {
        switch (record.id) {

        }
    }
}
