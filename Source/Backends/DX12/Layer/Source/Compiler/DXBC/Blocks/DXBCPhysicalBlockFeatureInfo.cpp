// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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
