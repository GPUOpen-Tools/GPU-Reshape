#include <Backends/DX12/Compiler/DXBC/Blocks/DXBCPhysicalBlockFeatureInfo.h>
#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXBC/DXBCParseContext.h>
#include <Backends/DX12/Compiler/DXIL/DXILModule.h>

void DXBCPhysicalBlockFeatureInfo::Parse() {
    // Block is optional
    DXBCPhysicalBlock* block = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::FeatureInfo);
    if (!block) {
        return;
    }

    // Setup parser
    DXBCParseContext ctx(block->ptr, block->length);

    // Resource offset
    features = DXBCShaderFeatureSet(ctx.Consume<uint64_t>());
}

void DXBCPhysicalBlockFeatureInfo::Compile() {
    // Block is optional
    DXBCPhysicalBlock* block = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::FeatureInfo);
    if (!block) {
        return;
    }

    // Get compiled binding info
    ASSERT(table.dxilModule, "PSV not supported for native DXBC");
    const DXILBindingInfo& bindingInfo = table.dxilModule->GetBindingInfo();

    if (bindingInfo.shaderFlags & DXILProgramShaderFlag::UseUAVs) {
        features |= DXBCShaderFeature::UAVsAtEveryStage;
    }

    if (bindingInfo.shaderFlags & DXILProgramShaderFlag::Use64UAVs) {
        features |= DXBCShaderFeature::Use64UAVs;
    }

    if (bindingInfo.shaderFlags & DXILProgramShaderFlag::UseRelaxedTypedUAVLoads) {
        features |= DXBCShaderFeature::TypedUAVLoadAdditionalFormats;
    }

    // Add features
    block->stream.Append(features.value);
}

void DXBCPhysicalBlockFeatureInfo::CopyTo(DXBCPhysicalBlockFeatureInfo &out) {
    out.features = features;
}
