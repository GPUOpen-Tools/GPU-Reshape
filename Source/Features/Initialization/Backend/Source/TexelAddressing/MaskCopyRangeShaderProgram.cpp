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

#include <Features/Initialization/TexelAddressing/MaskCopyRangeShaderProgram.h>
#include <Features/Initialization/TexelAddressing/MaskCopyRangeParameters.h>
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

MaskCopyRangeShaderProgram::MaskCopyRangeShaderProgram(ShaderDataID initializationMaskBufferID, IL::ResourceTokenType from, IL::ResourceTokenType to, bool isVolumetric) :
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
    IL::ConstantMap& constants = program.GetConstants();

    // Get entry point
    IL::Function* entryPoint = program.GetEntryPoint();
    
    // Must have termination block
    IL::BasicBlock* entryBlock = IL::GetTerminationBlock(program);
    if (!entryBlock) {
        return;
    }
        
    // Launch in shared configuration
    program.GetMetadataMap().AddMetadata(program.GetEntryPoint()->GetID(), kKernelSize);

    // Get the initialization buffer
    IL::ID initializationMaskBufferDataID = program.GetShaderDataMap().Get(initializationMaskBufferID)->id;
    
    // Get shader data
    IL::ShaderStruct<MaskCopyRangeParameters> data = program.GetShaderDataMap().Get(dataID)->id;

    // Create blocks
    IL::BasicBlock* exitInvalidDispatchBlock   = entryPoint->GetBasicBlocks().AllocBlock();
    IL::BasicBlock* exitInvalidAddressingBlock = entryPoint->GetBasicBlocks().AllocBlock();
    IL::BasicBlock* texelAddressBlock          = entryPoint->GetBasicBlocks().AllocBlock();
    IL::BasicBlock* writeBlock                 = entryPoint->GetBasicBlocks().AllocBlock();

    // Split the entry point for early out
    IL::BasicBlock::Iterator writeTerminatorIt = entryBlock->Split(writeBlock, entryBlock->GetTerminator());

    // Get dispatch offsets
    IL::Emitter<> entryEmitter(program, *entryBlock);
    IL::ID dispatchID = entryEmitter.KernelValue(IL::KernelValue::DispatchThreadID);
    IL::ID dispatchXID = entryEmitter.Extract(dispatchID, constants.UInt(0)->id);

    // Guard against the current chunk bounds (relative, not absolute)
    entryEmitter.BranchConditional(
        entryEmitter.LessThan(dispatchXID, data.Get<&MaskCopyRangeParameters::dispatchWidth>(entryEmitter)),
        texelAddressBlock,
        exitInvalidDispatchBlock,
        IL::ControlFlow::Selection(texelAddressBlock)
    );

    // If failed, just exit
    IL::Emitter<>(program, *exitInvalidDispatchBlock).Return();

    // Texel calculation emitter
    IL::Emitter<> texelEmitter(program, *texelAddressBlock);

    // Derive token information from shader data
    IL::StructResourceTokenEmitter sourceToken(texelEmitter, program.GetShaderDataMap().Get(sourceTokenID)->id);
    IL::StructResourceTokenEmitter destToken(texelEmitter, program.GetShaderDataMap().Get(destTokenID)->id);

    // Base dispatch offset
    dispatchXID = texelEmitter.Add(dispatchXID, data.Get<&MaskCopyRangeParameters::dispatchOffset>(texelEmitter));
    
    // Get memory base offsets
    IL::ID sourceBaseAlign32 = data.Get<&MaskCopyRangeParameters::sourceMemoryBaseElementAlign32>(texelEmitter);
    IL::ID destBaseAlign32 = data.Get<&MaskCopyRangeParameters::destMemoryBaseElementAlign32>(texelEmitter);

    // Final offsets
    IL::ID sourceTexel;
    IL::ID destTexel;

    // Setup subresource emitters
    InlineSubresourceEmitter sourceSubresourceEmitter(texelEmitter, sourceToken, texelEmitter.Load(initializationMaskBufferDataID), sourceBaseAlign32);
    InlineSubresourceEmitter destSubresourceEmitter(texelEmitter, destToken, texelEmitter.Load(initializationMaskBufferDataID), destBaseAlign32);
    
    // Symmetric copy?
    if (from == to) {
        // Buffer indexing just adds the linear offset
        if (from == IL::ResourceTokenType::Buffer) {
            sourceTexel = texelEmitter.Add(data.Get<&MaskCopyRangeParameters::sourceBaseX>(texelEmitter), dispatchXID);
            destTexel = texelEmitter.Add(data.Get<&MaskCopyRangeParameters::destBaseX>(texelEmitter), dispatchXID);
        } else {
            // Convert to 3d
            IL::TexelCoordinateScalar index = IL::TexelIndexTo3D(
                texelEmitter, dispatchXID,
                data.Get<&MaskCopyRangeParameters::width>(texelEmitter),
                data.Get<&MaskCopyRangeParameters::height>(texelEmitter),
                data.Get<&MaskCopyRangeParameters::depth>(texelEmitter)
            );
            
            // Compute the source intra-resource offset
            sourceTexel = IL::TexelAddressEmitter(texelEmitter, sourceToken, sourceSubresourceEmitter).LocalTextureTexelAddress(
                texelEmitter.Add(data.Get<&MaskCopyRangeParameters::sourceBaseX>(texelEmitter), index.x),
                texelEmitter.Add(data.Get<&MaskCopyRangeParameters::sourceBaseY>(texelEmitter), index.y),
                texelEmitter.Add(data.Get<&MaskCopyRangeParameters::sourceBaseZ>(texelEmitter), index.z),
                data.Get<&MaskCopyRangeParameters::sourceMip>(texelEmitter),
                isVolumetric
            ).texelOffset;

            // Compute the destination intra-resource offset
            destTexel = IL::TexelAddressEmitter(texelEmitter, destToken, destSubresourceEmitter).LocalTextureTexelAddress(
                texelEmitter.Add(data.Get<&MaskCopyRangeParameters::destBaseX>(texelEmitter), index.x),
                texelEmitter.Add(data.Get<&MaskCopyRangeParameters::destBaseY>(texelEmitter), index.y),
                texelEmitter.Add(data.Get<&MaskCopyRangeParameters::destBaseZ>(texelEmitter), index.z),
                data.Get<&MaskCopyRangeParameters::destMip>(texelEmitter),
                isVolumetric
            ).texelOffset;
        }
    } else {
        // Asymmetric copy, uses placement descriptors
        // Follows 1D scheduling with the total number of texels

        // Convert to 3d
        IL::TexelCoordinateScalar index = IL::TexelIndexTo3D(
            texelEmitter, dispatchXID,
            data.Get<&MaskCopyRangeParameters::width>(texelEmitter),
            data.Get<&MaskCopyRangeParameters::height>(texelEmitter),
            data.Get<&MaskCopyRangeParameters::depth>(texelEmitter)
        );
        
        // Placement dimensions
        IL::ID placementWidth  = data.Get<&MaskCopyRangeParameters::placementRowLength>(texelEmitter);
        IL::ID placementHeight = data.Get<&MaskCopyRangeParameters::placementImageHeight>(texelEmitter);
        
        // z * w * h + y * w + x
        IL::ID placementOffset = texelEmitter.Mul(index.z, texelEmitter.Mul(placementWidth, placementHeight));
        placementOffset = texelEmitter.Add(placementOffset, texelEmitter.Mul(index.y, placementWidth));
        placementOffset = texelEmitter.Add(placementOffset, index.x);
        
        // Buffer Placement -> Texture
        if (from == IL::ResourceTokenType::Buffer) {
            // Apply base offset to source placement
            sourceTexel = texelEmitter.Add(data.Get<&MaskCopyRangeParameters::sourceBaseX>(texelEmitter), placementOffset);
            
            // Compute the destination intra-resource offset
            destTexel = IL::TexelAddressEmitter(texelEmitter, destToken, destSubresourceEmitter).LocalTextureTexelAddress(
                texelEmitter.Add(data.Get<&MaskCopyRangeParameters::destBaseX>(texelEmitter), index.x),
                texelEmitter.Add(data.Get<&MaskCopyRangeParameters::destBaseY>(texelEmitter), index.y),
                texelEmitter.Add(data.Get<&MaskCopyRangeParameters::destBaseZ>(texelEmitter), index.z),
                data.Get<&MaskCopyRangeParameters::destMip>(texelEmitter),
                isVolumetric
            ).texelOffset;
        } else {
            // Texture -> Buffer Placement
            
            // Compute the source intra-resource offset
            sourceTexel = IL::TexelAddressEmitter(texelEmitter, sourceToken, sourceSubresourceEmitter).LocalTextureTexelAddress(
                texelEmitter.Add(data.Get<&MaskCopyRangeParameters::sourceBaseX>(texelEmitter), index.x),
                texelEmitter.Add(data.Get<&MaskCopyRangeParameters::sourceBaseY>(texelEmitter), index.y),
                texelEmitter.Add(data.Get<&MaskCopyRangeParameters::sourceBaseZ>(texelEmitter), index.z),
                data.Get<&MaskCopyRangeParameters::sourceMip>(texelEmitter),
                isVolumetric
            ).texelOffset;

            // Apply base offset to destination placement
            destTexel = texelEmitter.Add(data.Get<&MaskCopyRangeParameters::destBaseX>(texelEmitter), placementOffset);
        }
    }

    // Get the memory base offsets
    IL::ID sourceMemoryBase = sourceSubresourceEmitter.GetResourceMemoryBase();
    IL::ID destMemoryBase = destSubresourceEmitter.GetResourceMemoryBase();
    
    // Read the total number of texels
    IL::ID destResourceTexelCount = destSubresourceEmitter.ReadFieldDWord(TexelMemoryDWordFields::TexelCount);

    // Determine if the absolute offset is out of bounds
    texelEmitter.BranchConditional(
        texelEmitter.LessThan(destTexel, destResourceTexelCount),
        writeBlock,
        exitInvalidAddressingBlock,
        IL::ControlFlow::Selection(writeBlock)
    );

    // If failed, just exit
    IL::Emitter<>(program, *exitInvalidAddressingBlock).Return();
   
    // Append prior terminator
    IL::Emitter<> writeEmitter(program, *writeBlock, writeTerminatorIt);
    
    // Read the source initialization bit
    // This doesn't need to be atomic, as the source memory should be visible at this point
    IL::ID sourceBit = ReadTexelAddress(
        writeEmitter,
        initializationMaskBufferDataID,
        sourceMemoryBase,
        sourceTexel
    );

    // Write the source bit to the destination bit
    AtomicOrTexelAddressValue(
        writeEmitter,
        initializationMaskBufferDataID,
        destMemoryBase,
        destTexel,
        sourceBit
    );
}
