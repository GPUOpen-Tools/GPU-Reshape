#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockFunctionAttribute.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordReader.h>
#include <Backends/DX12/Compiler/DXIL/DXILIDMap.h>
#include <Backends/DX12/Compiler/DXIL/DXIL.Gen.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBitStreamReader.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 *
 * Loosely derived from LLVM BitcodeWriter
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/lib/Bitcode/Writer/BitcodeWriter.cpp
 */

void DXILPhysicalBlockFunctionAttribute::CopyTo(DXILPhysicalBlockFunctionAttribute &out) {
    out.parameterAttributeGroups = parameterAttributeGroups;
    out.parameterGroups = parameterGroups;
}

void DXILPhysicalBlockFunctionAttribute::ParseParameterAttributeGroup(struct LLVMBlock *block) {
    for (const LLVMRecord& record : block->records) {
        switch (record.id) {
            case static_cast<uint32_t>(LLVMParameterGroupRecord::Entry): {
                LLVMRecordReader reader(record);

                /*
                 * LLVM Specification
                 *   [ENTRY, grpid, paramidx, attr0, attr1, ...]
                 * */

                ParameterAttributeGroup& group = parameterAttributeGroups.Add();

                uint32_t id = reader.ConsumeOp();
                uint64_t parameter = reader.ConsumeOp();

                switch (static_cast<LLVMParameterGroupRecordIndex>(parameter)) {
                    default:
                        break;
                    case LLVMParameterGroupRecordIndex::Return:
                        break;
                    case LLVMParameterGroupRecordIndex::FunctionAttribute:
                        break;
                }

                while (reader.Any()) {
                    /*
                     * LLVM Specification
                     *   kind, key [, ...], [value [, ...]]
                     * */

                    auto kind = reader.ConsumeOpAs<LLVMParameterGroupKind>();

                    ParameterAttribute& attribute = group.attributes.Add();

                    switch (kind) {
                        case LLVMParameterGroupKind::WellKnown:{
                            attribute.value = reader.ConsumeOpAs<LLVMParameterGroupValue>();
                            break;
                        }
                        case LLVMParameterGroupKind::WellKnownValue: {
                            uint64_t key = reader.ConsumeOp();
                            attribute.value = reader.ConsumeOpAs<LLVMParameterGroupValue>();
                            break;
                        }
                        case LLVMParameterGroupKind::String: {
                            attribute.view = LLVMRecordStringView(record, reader.Offset());
                            break;
                        }
                        case LLVMParameterGroupKind::StringValue: {
                            uint64_t key = reader.ConsumeOp();
                            attribute.view = LLVMRecordStringView(record, reader.Offset());
                            break;
                        }
                    };
                }
            }
                break;
        }
    }
}

void DXILPhysicalBlockFunctionAttribute::ParseParameterBlock(struct LLVMBlock *block) {
    for (const LLVMRecord& record : block->records) {
        switch (record.id) {
            case static_cast<uint32_t>(LLVMParameterRecord::Entry): {
                LLVMRecordReader reader(record);

                /*
                 * LLVM Specification
                 *   [ENTRY, attrgrp0, attrgrp1, ...]
                 */

                ParameterGroup& group = parameterGroups.Add();

                while (reader.Any()) {
                    uint32_t index = reader.ConsumeOp();
                    ASSERT(index > 0, "Invalid parameter group index");

                    for (const ParameterAttribute& attrib : parameterAttributeGroups[index - 1].attributes) {
                        group.attributes.Add(attrib);
                    }
                }
                break;
            }
        }
    }
}

uint32_t DXILPhysicalBlockFunctionAttribute::FindOrCompileAttributeList(uint32_t count, const LLVMParameterGroupValue *values) {
    for (uint32_t groupIdx = 0; groupIdx < parameterGroups.Size(); groupIdx++) {
        const ParameterGroup& group = parameterGroups[groupIdx];

        // Count mismatch?
        if (group.attributes.Size() != count) {
            continue;
        }

        bool mismatch = false;

        // Match all parameters
        for (size_t i = 0; i < count; i++) {
            if (group.attributes[i].value != values[i]) {
                mismatch = true;
            }
        }

        // OK?
        if (!mismatch) {
            return groupIdx + 1;
        }
    }

    // Destination group index
    uint32_t groupIndex = parameterAttributeGroups.Size();

    // Allocate group
    ParameterAttributeGroup& group = parameterAttributeGroups.Add();

    // Group record
    LLVMRecord groupRecord(LLVMParameterGroupRecord::Entry);
    groupRecord.opCount = 2 + 2 * count;
    groupRecord.ops = table.recordAllocator.AllocateArray<uint64_t>(groupRecord.opCount);
    groupRecord.ops[0] = parameterAttributeGroups.Size();
    groupRecord.ops[1] = static_cast<uint64_t>(LLVMParameterGroupRecordIndex::FunctionAttribute);

    // Add attributes
    for (size_t i = 0; i < count; i++) {
        groupRecord.ops[2 + i * 2] = static_cast<uint64_t>(LLVMParameterGroupKind::WellKnown);
        groupRecord.ops[3 + i * 2] = static_cast<uint64_t>(values[i]);

        ParameterAttribute& attribute = group.attributes.Add();
        attribute.value = values[i];
    }

    // Add group record
    groupDeclarationBlock->AddRecord(groupRecord);

    // Destination parameter index
    uint32_t parameterIndex = parameterGroups.Size();

    // Parameter record
    LLVMRecord parameterRecord(LLVMParameterRecord::Entry);
    parameterRecord.opCount = 1;
    parameterRecord.ops = table.recordAllocator.AllocateArray<uint64_t>(1);
    parameterRecord.ops[0] = 1u + groupIndex;
    parameterDeclarationBlock->AddRecord(parameterRecord);

    // OK
    return 1u + parameterIndex;
}

void DXILPhysicalBlockFunctionAttribute::SetDeclarationBlock(struct LLVMBlock *block) {
    parameterDeclarationBlock = block->GetBlock(LLVMReservedBlock::Parameter);
    if (!parameterDeclarationBlock) {
        parameterDeclarationBlock = new (allocators) LLVMBlock(LLVMReservedBlock::Parameter);
        parameterDeclarationBlock->abbreviationSize = 4u;
        block->InsertBlock(block->FindPlacement(LLVMBlockElementType::Block), parameterDeclarationBlock);
    }

    groupDeclarationBlock = block->GetBlock(LLVMReservedBlock::ParameterGroup);
    if (!groupDeclarationBlock) {
        groupDeclarationBlock = new (allocators) LLVMBlock(LLVMReservedBlock::ParameterGroup);
        groupDeclarationBlock->abbreviationSize = 4u;
        block->InsertBlock(block->FindPlacement(LLVMBlockElementType::Block), groupDeclarationBlock);
    }
}
