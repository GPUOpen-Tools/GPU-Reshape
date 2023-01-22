#include <Backends/DX12/Compiler/DXBC/Blocks/DXBCPhysicalBlockRootSignature.h>
#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXBC/DXBCParseContext.h>
#include <Backends/DX12/Compiler/DXIL/DXILModule.h>

void DXBCPhysicalBlockRootSignature::Parse() {
    // Block is optional
    DXBCPhysicalBlock *block = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::RootSignature);
    if (!block) {
        return;
    }

    // Setup parser
    DXBCParseContext ctx(block->ptr, block->length);

    // Get the header
    header = ctx.Consume<DXBCRootSignatureHeader>();

    // Get parameter start
    const auto *parameterStart = reinterpret_cast<const DXILRootSignatureParameter *>(block->ptr + header.rootParameterOffset);

    // Read all parameters
    for (uint32_t i = 0; i < header.parameterCount; i++) {
        const DXILRootSignatureParameter &source = parameterStart[i];

        // Create parameter
        RootParameter &parameter = parameters.emplace_back();
        parameter.type = source.type;
        parameter.visibility = source.visibility;

        // Handle type
        switch (parameter.type) {
            case DXBCRootSignatureParameterType::DescriptorTable: {
                const auto *table = reinterpret_cast<const DXBCRootSignatureDescriptorTable *>(block->ptr + source.payloadOffset);

                // Version 1 deserialization?
                if (header.version == DXBCRootSignatureVersion::Version1) {
                    const auto *rangeStart = reinterpret_cast<const DXBCRootSignatureDescriptorRange1 *>(block->ptr + table->rangeOffset);

                    // Just push all data
                    parameter.descriptorTable.ranges.insert(parameter.descriptorTable.ranges.end(), rangeStart, rangeStart + table->rangeCount);
                } else {
                    const auto *rangeStart = reinterpret_cast<const DXBCRootSignatureDescriptorRange *>(block->ptr + table->rangeOffset);

                    // Convert version 0 to 1
                    for (uint32_t rangeIndex = 0; rangeIndex < table->rangeCount; rangeIndex++) {
                        const DXBCRootSignatureDescriptorRange &sourceRange = rangeStart[rangeIndex];

                        // Add promoted range
                        DXBCRootSignatureDescriptorRange1 range1{};
                        range1.type = sourceRange.type;
                        range1.descriptorCount = sourceRange.descriptorCount;
                        range1._register = sourceRange._register;
                        range1.space = sourceRange.space;
                        range1.offsetFromTableStart = sourceRange.offsetFromTableStart;
                        parameter.descriptorTable.ranges.push_back(range1);
                    }
                }
                break;
            }
            case DXBCRootSignatureParameterType::Constant32: {
                // Just push all data
                parameter.constant = *reinterpret_cast<const DXBCRootSignatureConstant *>(block->ptr + source.payloadOffset);
                break;
            }
            case DXBCRootSignatureParameterType::CBV:
            case DXBCRootSignatureParameterType::SRV:
            case DXBCRootSignatureParameterType::UAV:
                // Just push all data
                parameter.parameter1 = *reinterpret_cast<const DXBCRootSignatureParameter1 *>(block->ptr + source.payloadOffset);
                break;
        }
    }

    // Get sampler start
    const auto *samplerStart = reinterpret_cast<const DXBCRootSignatureSamplerStub *>(block->ptr + header.staticSamplerOffset);

    // Append samplers
    samplers.insert(samplers.end(), samplerStart, samplerStart + header.staticSamplerCount);
}

void DXBCPhysicalBlockRootSignature::CompileShaderExport() {
    // Get compiled binding info
    ASSERT(table.dxilModule, "RS not supported for native DXBC");
    const RootRegisterBindingInfo& bindingInfo = table.dxilModule->GetBindingInfo().bindingInfo;

    // Shader export
    {
        // Create parameter
        RootParameter& parameter = parameters.emplace_back();
        parameter.type = DXBCRootSignatureParameterType::DescriptorTable;
        parameter.visibility = DXBCRootSignatureVisibility::All;

        // Create export range
        DXBCRootSignatureDescriptorRange1& exportRange = parameter.descriptorTable.ranges.emplace_back();
        exportRange.type = DXBCRootSignatureRangeType::UAV;
        exportRange.space = bindingInfo.space;
        exportRange._register = bindingInfo.shaderExportBaseRegister;
        exportRange.descriptorCount = bindingInfo.shaderExportCount;
        exportRange.flags = 0x0;
        exportRange.offsetFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // Create PRMT range
        DXBCRootSignatureDescriptorRange1& prmtRange = parameter.descriptorTable.ranges.emplace_back();
        prmtRange.type = DXBCRootSignatureRangeType::SRV;
        prmtRange.space = bindingInfo.space;
        prmtRange._register = bindingInfo.prmtBaseRegister;
        prmtRange.descriptorCount = 1u;
        prmtRange.flags = 0x0;
        prmtRange.offsetFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // Create user range
        DXBCRootSignatureDescriptorRange1& userRange = parameter.descriptorTable.ranges.emplace_back();
        userRange.type = DXBCRootSignatureRangeType::UAV;
        userRange.space = bindingInfo.space;
        userRange._register = bindingInfo.shaderResourceBaseRegister;
        userRange.descriptorCount = bindingInfo.shaderResourceCount;
        userRange.flags = 0x0;
        userRange.offsetFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    // Descriptor data
    {
        // Create parameter
        RootParameter& parameter = parameters.emplace_back();
        parameter.type = DXBCRootSignatureParameterType::CBV;
        parameter.visibility = DXBCRootSignatureVisibility::All;
        parameter.parameter1.space = bindingInfo.space;
        parameter.parameter1._register = bindingInfo.descriptorConstantBaseRegister;
    }

    // Event data
    {
        // Create parameter
        RootParameter& parameter = parameters.emplace_back();
        parameter.type = DXBCRootSignatureParameterType::Constant32;
        parameter.visibility = DXBCRootSignatureVisibility::All;
        parameter.constant.space = bindingInfo.space;
        parameter.constant._register = bindingInfo.eventConstantBaseRegister;
        parameter.constant.dwordCount = 1u;
    }
}

void DXBCPhysicalBlockRootSignature::Compile() {
    // Block is optional
    DXBCPhysicalBlock *block = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::RootSignature);
    if (!block) {
        return;
    }

    // Emit instrumented information
    CompileShaderExport();

    // Emit partially updated header
    header.parameterCount = static_cast<uint32_t>(parameters.size());
    header.staticSamplerCount = static_cast<uint32_t>(samplers.size());
    const size_t headerOffset = block->stream.Append(header);

    // Range size
    const uint32_t rangeSize = (header.version == DXBCRootSignatureVersion::Version1) ? sizeof(DXBCRootSignatureDescriptorRange1) : sizeof(DXBCRootSignatureDescriptorRange);

    // Emit all data from leaf to root, after the header has been emitted
    uint32_t rangeOffset = block->stream.GetOffset();

    // Emit ranges
    for (const RootParameter &parameter: parameters) {
        if (parameter.type != DXBCRootSignatureParameterType::DescriptorTable) {
            continue;
        }

        // Write versioned ranges
        if (header.version == DXBCRootSignatureVersion::Version1) {
            for (const DXBCRootSignatureDescriptorRange1 &range1: parameter.descriptorTable.ranges) {
                block->stream.Append(range1);
            }
        } else {
            for (const DXBCRootSignatureDescriptorRange1 &range1: parameter.descriptorTable.ranges) {
                DXBCRootSignatureDescriptorRange range{};
                range.type = range1.type;
                range.descriptorCount = range1.descriptorCount;
                range._register = range1._register;
                range.space = range1.space;
                range.offsetFromTableStart = range1.offsetFromTableStart;
                block->stream.Append(range);
            }
        }
    }

    // Validation data
    uint32_t rangeEnd = block->stream.GetOffset();

    // Offset for all tables
    uint32_t descriptorTableOffset = block->stream.GetOffset();

    // Emit parameters
    for (const RootParameter &parameter: parameters) {
        if (parameter.type == DXBCRootSignatureParameterType::DescriptorTable) {
            // Emit table
            DXBCRootSignatureDescriptorTable table;
            table.rangeCount = static_cast<uint32_t>(parameter.descriptorTable.ranges.size());
            table.rangeOffset = rangeOffset;
            block->stream.Append(table);

            // Increment range offset
            rangeOffset += rangeSize * table.rangeCount;
        }
    }

    // Validation data
    uint32_t descriptorTableEnd = block->stream.GetOffset();

    // Offset for all constants
    uint32_t constantOffset = block->stream.GetOffset();

    // Emit constants
    for (const RootParameter &parameter: parameters) {
        if (parameter.type == DXBCRootSignatureParameterType::Constant32) {
            block->stream.Append(parameter.constant);
        }
    }

    // Validation data
    uint32_t constantEnd = block->stream.GetOffset();

    // Offset for all parameters
    uint32_t parameterOffset = block->stream.GetOffset();

    // Emit root parameters
    for (const RootParameter &parameter: parameters) {
        if (parameter.type == DXBCRootSignatureParameterType::CBV ||
            parameter.type == DXBCRootSignatureParameterType::SRV ||
            parameter.type == DXBCRootSignatureParameterType::UAV) {
            block->stream.Append(parameter.parameter1);
        }
    }

    // Validation data
    uint32_t parameterEnd = block->stream.GetOffset();

    // Offset for all root parameters
    uint32_t rootParameterHeaderOffset = block->stream.GetOffset();

    // Emit parameters
    for (const RootParameter &source: parameters) {
        DXILRootSignatureParameter parameter;
        parameter.type = source.type;
        parameter.visibility = source.visibility;

        // Set payload
        switch (source.type) {
            case DXBCRootSignatureParameterType::DescriptorTable:
                parameter.payloadOffset = descriptorTableOffset;
                descriptorTableOffset += sizeof(DXBCRootSignatureDescriptorTable);
                break;
            case DXBCRootSignatureParameterType::Constant32:
                parameter.payloadOffset = constantOffset;
                constantOffset += sizeof(DXBCRootSignatureConstant);
                break;
            case DXBCRootSignatureParameterType::CBV:
            case DXBCRootSignatureParameterType::SRV:
            case DXBCRootSignatureParameterType::UAV:
                parameter.payloadOffset = parameterOffset;
                parameterOffset += sizeof(DXBCRootSignatureParameter1);
                break;
        }

        // Finally apend
        block->stream.Append(parameter);
    }

    // Validation
    ASSERT(rangeOffset == rangeEnd, "Misconsumed root signature ranges");
    ASSERT(descriptorTableOffset == descriptorTableEnd, "Misconsumed root signature ranges");
    ASSERT(constantOffset == constantEnd, "Misconsumed root signature ranges");
    ASSERT(parameterOffset == parameterEnd, "Misconsumed root signature ranges");

    // Offset for all samplers
    uint32_t samplerOffset = block->stream.GetOffset();

    // Blind insert all samplers
    block->stream.AppendData(samplers.data(), static_cast<uint32_t>(samplers.size() * sizeof(DXBCRootSignatureSamplerStub)));

    // Update header
    auto *mutableHeader = block->stream.GetMutableDataAt<DXBCRootSignatureHeader>(headerOffset);
    mutableHeader->rootParameterOffset = rootParameterHeaderOffset;
    mutableHeader->staticSamplerOffset = samplerOffset;
}

void DXBCPhysicalBlockRootSignature::CopyTo(DXBCPhysicalBlockRootSignature &out) {
    out.header = header;
    out.parameters = parameters;
    out.samplers = samplers;
}
