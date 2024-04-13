// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
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

#include <Features/Initialization/MaskCopyRangeShaderProgram.h>
#include <Features/Initialization/BitIndexing.h>
#include <Features/Initialization/MaskCopyRangeParameters.h>

// Backend
#include <Backend/IL/ProgramCommon.h>
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/IL/Emitter.h>
#include <Backend/IL/ShaderStruct.h>
#include <Backend/IL/Devices/StructResourceTokenEmitter.h>
#include <Backend/Resource/TexelAddressEmitter.h>

// Common
#include <Common/Registry.h>

MaskCopyRangeShaderProgram::MaskCopyRangeShaderProgram(ShaderDataID initializationMaskBufferID, Backend::IL::ResourceTokenType from, Backend::IL::ResourceTokenType to, bool isVolumetric) :
    initializationMaskBufferID(initializationMaskBufferID),
    from(from), to(to),
    isVolumetric(isVolumetric)  {

}

bool MaskCopyRangeShaderProgram::Install() {
    // Shader data host
    shaderDataHost = registry->Get<IShaderDataHost>();

    // Create event data
    dataID = shaderDataHost->CreateDescriptorData(ShaderDataDescriptorInfo::FromStruct<MaskCopyRangeParameters>());
    sourceTokenID = shaderDataHost->CreateDescriptorData(ShaderDataDescriptorInfo::FromStruct<ResourceToken>());
    destTokenID = shaderDataHost->CreateDescriptorData(ShaderDataDescriptorInfo::FromStruct<ResourceToken>());

    // OK
    return true;
}

void MaskCopyRangeShaderProgram::Inject(IL::Program &program) {
    Backend::IL::ConstantMap& constants = program.GetConstants();

    // Must have termination block
    IL::BasicBlock* basicBlock = Backend::IL::GetTerminationBlock(program);
    if (!basicBlock) {
        return;
    }

    // Get the initialization buffer
    IL::ID initializationMaskBufferDataID = program.GetShaderDataMap().Get(initializationMaskBufferID)->id;
    
    // Get shader data
    IL::ShaderStruct<MaskCopyRangeParameters> data = program.GetShaderDataMap().Get(dataID)->id;

    // Append prior terminator
    IL::Emitter<> emitter(program, *basicBlock, basicBlock->GetTerminator());

    // Derive token information from shader data
    IL::StructResourceTokenEmitter sourceToken(emitter, program.GetShaderDataMap().Get(sourceTokenID)->id);
    IL::StructResourceTokenEmitter destToken(emitter, program.GetShaderDataMap().Get(destTokenID)->id);

    // todo[init]: guard oob dtid

    // Get dispatch offsets
    IL::ID dispatchID = emitter.KernelValue(Backend::IL::KernelValue::DispatchThreadID);
    IL::ID dispatchXID = emitter.Extract(dispatchID, constants.UInt(0)->id);
    IL::ID dispatchYID = emitter.Extract(dispatchID, constants.UInt(1)->id);
    IL::ID dispatchZID = emitter.Extract(dispatchID, constants.UInt(2)->id);
    
    // Get memory base offsets
    IL::ID sourceBaseAlign32 = data.Get<&MaskCopyRangeParameters::sourceMemoryBaseElementAlign32>(emitter);
    IL::ID destBaseAlign32 = data.Get<&MaskCopyRangeParameters::destMemoryBaseElementAlign32>(emitter);

    // Final offsets
    IL::ID sourceTexel;
    IL::ID destTexel;

    // Symmetric copy?
    if (from == to) {
        // Buffer indexing just adds the linear offset
        if (from == Backend::IL::ResourceTokenType::Buffer) {
            sourceTexel = emitter.Add(data.Get<&MaskCopyRangeParameters::sourceBaseX>(emitter), dispatchXID);
            destTexel = emitter.Add(data.Get<&MaskCopyRangeParameters::destBaseX>(emitter), dispatchXID);
        } else {
            // Compute the source intra-resource offset
            sourceTexel = Backend::IL::TexelAddressEmitter(emitter, sourceToken).LocalTexelAddress(
                emitter.Add(data.Get<&MaskCopyRangeParameters::sourceBaseX>(emitter), dispatchXID),
                emitter.Add(data.Get<&MaskCopyRangeParameters::sourceBaseY>(emitter), dispatchYID),
                emitter.Add(data.Get<&MaskCopyRangeParameters::sourceBaseZ>(emitter), dispatchZID),
                data.Get<&MaskCopyRangeParameters::sourceMip>(emitter),
                isVolumetric
            );

            // Compute the destination intra-resource offset
            destTexel = Backend::IL::TexelAddressEmitter(emitter, destToken).LocalTexelAddress(
                emitter.Add(data.Get<&MaskCopyRangeParameters::destBaseX>(emitter), dispatchXID),
                emitter.Add(data.Get<&MaskCopyRangeParameters::destBaseY>(emitter), dispatchYID),
                emitter.Add(data.Get<&MaskCopyRangeParameters::destBaseZ>(emitter), dispatchZID),
                data.Get<&MaskCopyRangeParameters::destMip>(emitter),
                isVolumetric
            );
        }
    } else {
        // Asymmetric copy, uses placement descriptors
        // Follows 1D scheduling with the total number of texels

        // Placement dimensions
        IL::ID placementWidth  = data.Get<&MaskCopyRangeParameters::placementRowLength>(emitter);
        IL::ID placementHeight = data.Get<&MaskCopyRangeParameters::placementImageHeight>(emitter);
        
        // z * w * h + y * w + x
        IL::ID placementOffset = emitter.Mul(dispatchZID, emitter.Mul(placementWidth, placementHeight));
        placementOffset = emitter.Add(placementOffset, emitter.Mul(dispatchYID, placementWidth));
        placementOffset = emitter.Add(placementOffset, dispatchXID);
        
        // Buffer Placement -> Texture
        if (from == Backend::IL::ResourceTokenType::Buffer) {
            // Apply base offset to source placement
            sourceTexel = emitter.Add(data.Get<&MaskCopyRangeParameters::sourceBaseX>(emitter), placementOffset);
            
            // Compute the destination intra-resource offset
            destTexel = Backend::IL::TexelAddressEmitter(emitter, destToken).LocalTexelAddress(
                emitter.Add(data.Get<&MaskCopyRangeParameters::destBaseX>(emitter), dispatchXID),
                emitter.Add(data.Get<&MaskCopyRangeParameters::destBaseY>(emitter), dispatchYID),
                emitter.Add(data.Get<&MaskCopyRangeParameters::destBaseZ>(emitter), dispatchZID),
                data.Get<&MaskCopyRangeParameters::destMip>(emitter),
                isVolumetric
            );
        } else {
            // Texture -> Buffer Placement
            
            // Compute the source intra-resource offset
            sourceTexel = Backend::IL::TexelAddressEmitter(emitter, sourceToken).LocalTexelAddress(
                emitter.Add(data.Get<&MaskCopyRangeParameters::sourceBaseX>(emitter), dispatchXID),
                emitter.Add(data.Get<&MaskCopyRangeParameters::sourceBaseY>(emitter), dispatchYID),
                emitter.Add(data.Get<&MaskCopyRangeParameters::sourceBaseZ>(emitter), dispatchZID),
                data.Get<&MaskCopyRangeParameters::sourceMip>(emitter),
                isVolumetric
            );

            // Apply base offset to destination placement
            destTexel = emitter.Add(data.Get<&MaskCopyRangeParameters::destBaseX>(emitter), placementOffset);
        }
    }

    // Read the source initialization bit
    IL::ID sourceBit = AtomicAndTexelAddress(emitter, initializationMaskBufferDataID, sourceBaseAlign32, sourceTexel);

    // Write the source bit to the destination bit
    AtomicOrTexelAddress(emitter, initializationMaskBufferDataID, destBaseAlign32, destTexel, sourceBit);
}
