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

#include <Features/Initialization/MaskBlitShaderProgram.h>
#include <Features/Initialization/MaskBlitParameters.h>
#include <Features/Initialization/KernelShared.h>

// Backend
#include <Backend/IL/ProgramCommon.h>
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/IL/Emitters/Emitter.h>
#include <Backend/IL/ShaderStruct.h>
#include <Backend/IL/Emitters/StructResourceTokenEmitter.h>

// Addressing
#include <Addressing/IL/BitIndexing.h>
#include <Addressing/IL/Emitters/InlineSubresourceEmitter.h>
#include <Addressing/IL/Emitters/TexelAddressEmitter.h>

// Common
#include <Common/Registry.h>

MaskBlitShaderProgram::MaskBlitShaderProgram(ShaderDataID initializationMaskBufferID, Backend::IL::ResourceTokenType type, bool isVolumetric) :
    initializationMaskBufferID(initializationMaskBufferID),
    type(type),
    isVolumetric(isVolumetric) {

}

bool MaskBlitShaderProgram::Install() {
    // Shader data host
    shaderDataHost = registry->Get<IShaderDataHost>();

    // Create data
    dataID = shaderDataHost->CreateDescriptorData(ShaderDataDescriptorInfo::FromStruct<MaskBlitParameters>());
    destTokenID = shaderDataHost->CreateDescriptorData(ShaderDataDescriptorInfo::FromStruct<ResourceToken>());

    // OK
    return true;
}

void MaskBlitShaderProgram::Inject(IL::Program &program) {
    Backend::IL::ConstantMap& constants = program.GetConstants();

    // Must have termination block
    IL::BasicBlock* basicBlock = Backend::IL::GetTerminationBlock(program);
    if (!basicBlock) {
        return;
    }
        
    // Launch in shared configuration
    program.GetMetadataMap().AddMetadata(program.GetEntryPoint()->GetID(), kKernelSize);

    // Get the initialization buffer
    IL::ID initializationMaskBufferDataID = program.GetShaderDataMap().Get(initializationMaskBufferID)->id;

    // Get shader data
    IL::ShaderStruct<MaskBlitParameters> data = program.GetShaderDataMap().Get(dataID)->id;

    // Append prior terminator
    IL::Emitter<> emitter(program, *basicBlock, basicBlock->GetTerminator());

    // Derive token information from shader data
    IL::StructResourceTokenEmitter token(emitter, program.GetShaderDataMap().Get(destTokenID)->id);

    // todo[init]: guard oob dtid

    // Get dispatch offsets
    IL::ID dispatchID = emitter.KernelValue(Backend::IL::KernelValue::DispatchThreadID);
    IL::ID dispatchXID = emitter.Extract(dispatchID, constants.UInt(0)->id);

    // Base dispatch offset
    dispatchXID = emitter.Add(dispatchXID, data.Get<&MaskBlitParameters::dispatchOffset>(emitter));
    
    // Get memory base offset
    IL::ID baseAlign32 = data.Get<&MaskBlitParameters::memoryBaseElementAlign32>(emitter);

    // Final offset
    IL::ID texel;

    // Setup subresource emitter
    InlineSubresourceEmitter subresourceEmitter(emitter, token, emitter.Load(initializationMaskBufferDataID), baseAlign32);

    // Buffer indexing just adds the linear offset
    if (type == Backend::IL::ResourceTokenType::Buffer) {
        texel = emitter.Add(data.Get<&MaskBlitParameters::baseX>(emitter), dispatchXID);
    } else {
        // Texel addressing computation
        Backend::IL::TexelAddressEmitter address(emitter, token, subresourceEmitter);

        // Convert to 3d
        Backend::IL::TexelCoordinateScalar index = Backend::IL::TexelIndexTo3D(
            emitter, dispatchXID,
            data.Get<&MaskBlitParameters::width>(emitter),
            data.Get<&MaskBlitParameters::height>(emitter),
            data.Get<&MaskBlitParameters::depth>(emitter)
        );
        
        // Compute the intra-resource offset
        texel = address.LocalTextureTexelAddress(
            emitter.Add(data.Get<&MaskBlitParameters::baseX>(emitter), index.x),
            emitter.Add(data.Get<&MaskBlitParameters::baseY>(emitter), index.y),
            emitter.Add(data.Get<&MaskBlitParameters::baseZ>(emitter), index.z),
            data.Get<&MaskBlitParameters::mip>(emitter),
            isVolumetric
        ).texelOffset;
    }

    // Mark the given texel as initialized
    
    // Blitting operates on a per-block basis, it assumes thread safety

    // todo[init]: fix this!
    WriteTexelAddressBlock(emitter, initializationMaskBufferDataID, subresourceEmitter.GetResourceMemoryBase(), emitter.Div(texel, constants.UInt(32)->id), constants.UInt(~0u)->id);
    // AtomicOrTexelAddress(emitter, initializationMaskBufferDataID, baseAlign32, texel);
}
