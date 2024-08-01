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

#include <Features/Initialization/TexelAddressing/MaskBlitShaderProgram.h>
#include <Features/Initialization/TexelAddressing/MaskBlitParameters.h>
#include <Features/Initialization/TexelAddressing/KernelShared.h>

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

    // Get entry point
    IL::Function* entryPoint = program.GetEntryPoint();
    
    // Must have termination block
    IL::BasicBlock* entryBlock = Backend::IL::GetTerminationBlock(program);
    if (!entryBlock) {
        return;
    }

    // Launch in shared configuration
    program.GetMetadataMap().AddMetadata(entryPoint->GetID(), kKernelSize);

    // Get the initialization buffer
    IL::ID initializationMaskBufferDataID = program.GetShaderDataMap().Get(initializationMaskBufferID)->id;

    // Get shader data
    IL::ShaderStruct<MaskBlitParameters> data = program.GetShaderDataMap().Get(dataID)->id;

    // Create blocks
    IL::BasicBlock* exitInvalidDispatchBlock   = entryPoint->GetBasicBlocks().AllocBlock();
    IL::BasicBlock* exitInvalidAddressingBlock = entryPoint->GetBasicBlocks().AllocBlock();
    IL::BasicBlock* texelAddressBlock          = entryPoint->GetBasicBlocks().AllocBlock();
    IL::BasicBlock* writeBlock                 = entryPoint->GetBasicBlocks().AllocBlock();

    // Split the entry point for early out
    IL::BasicBlock::Iterator writeTerminatorIt = entryBlock->Split(writeBlock, entryBlock->GetTerminator());

    // Get dispatch offsets
    IL::Emitter<> entryEmitter(program, *entryBlock);
    IL::ID dispatchID = entryEmitter.KernelValue(Backend::IL::KernelValue::DispatchThreadID);
    IL::ID dispatchXID = entryEmitter.Extract(dispatchID, constants.UInt(0)->id);

    // Guard against the current chunk bounds (relative, not absolute)
    entryEmitter.BranchConditional(
        entryEmitter.LessThan(dispatchXID, data.Get<&MaskBlitParameters::dispatchWidth>(entryEmitter)),
        texelAddressBlock,
        exitInvalidDispatchBlock,
        IL::ControlFlow::Selection(texelAddressBlock)
    );

    // If failed, just exit
    IL::Emitter<>(program, *exitInvalidDispatchBlock).Return();

    // Texel calculation emitter
    IL::Emitter<> texelEmitter(program, *texelAddressBlock);

    // Derive token information from shader data
    IL::StructResourceTokenEmitter token(texelEmitter, program.GetShaderDataMap().Get(destTokenID)->id);

    // Base dispatch offset
    dispatchXID = texelEmitter.Add(dispatchXID, data.Get<&MaskBlitParameters::dispatchOffset>(texelEmitter));
    
    // Get memory base offset
    IL::ID baseAlign32 = data.Get<&MaskBlitParameters::memoryBaseElementAlign32>(texelEmitter);

    // Final offset
    IL::ID texelOffset;

    // Setup subresource emitter
    InlineSubresourceEmitter subresourceEmitter(texelEmitter, token, texelEmitter.Load(initializationMaskBufferDataID), baseAlign32);

    // Buffer indexing just adds the linear offset
    if (type == Backend::IL::ResourceTokenType::Buffer) {
        texelOffset = texelEmitter.Add(data.Get<&MaskBlitParameters::baseX>(texelEmitter), dispatchXID);
    } else {
        // Texel addressing computation
        Backend::IL::TexelAddressEmitter address(texelEmitter, token, subresourceEmitter);

        // Convert to 3d
        Backend::IL::TexelCoordinateScalar index = Backend::IL::TexelIndexTo3D(
            texelEmitter, dispatchXID,
            data.Get<&MaskBlitParameters::width>(texelEmitter),
            data.Get<&MaskBlitParameters::height>(texelEmitter),
            data.Get<&MaskBlitParameters::depth>(texelEmitter)
        );
        
        // Compute the intra-resource offset
        texelOffset = address.LocalTextureTexelAddress(
            texelEmitter.Add(data.Get<&MaskBlitParameters::baseX>(texelEmitter), index.x),
            texelEmitter.Add(data.Get<&MaskBlitParameters::baseY>(texelEmitter), index.y),
            texelEmitter.Add(data.Get<&MaskBlitParameters::baseZ>(texelEmitter), index.z),
            data.Get<&MaskBlitParameters::mip>(texelEmitter),
            isVolumetric
        ).texelOffset;
    }
    
    // Get the memory base offset
    IL::ID memoryBase = subresourceEmitter.GetResourceMemoryBase();
    
    // Read the total number of texels
    IL::ID resourceTexelCount = subresourceEmitter.ReadFieldDWord(TexelMemoryDWordFields::TexelCount);

    // Determine if the absolute offset is out of bounds
    texelEmitter.BranchConditional(
        texelEmitter.LessThan(texelOffset, resourceTexelCount),
        writeBlock,
        exitInvalidAddressingBlock,
        IL::ControlFlow::Selection(writeBlock)
    );

    // If failed, just exit
    IL::Emitter<>(program, *exitInvalidAddressingBlock).Return();
   
    // Append prior terminator
    IL::Emitter<> writeEmitter(program, *writeBlock, writeTerminatorIt);
    
    // Mark the given texel as initialized
    // Blitting operates on a per-block basis, it assumes thread safety
    // todo[init]: fix this!
    WriteTexelAddressBlock(
        writeEmitter,
        initializationMaskBufferDataID,
        memoryBase,
        writeEmitter.Div(texelOffset, constants.UInt(32)->id),
        constants.UInt(~0u)->id
    );
    
    // AtomicOrTexelAddress(emitter, initializationMaskBufferDataID, baseAlign32, texel);
}
