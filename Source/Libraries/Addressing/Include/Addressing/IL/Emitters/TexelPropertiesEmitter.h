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

#pragma once

// Addressing
#include <Addressing/IL/Emitters/TexelAddressEmitter.h>
#include <Addressing/IL/Emitters/InlineSubresourceEmitter.h>
#include <Addressing/IL/TexelProperties.h>
#include <Addressing/TexelMemoryAllocator.h>

// Backend
#include <Backend/IL/Emitters/Emitter.h>
#include <Backend/IL/Emitters/ExtendedEmitter.h>
#include <Backend/IL/Emitters/ResourceTokenEmitter.h>

namespace Backend::IL {
    template<typename E = Emitter<>>
    struct TexelPropertiesEmitter {
        static constexpr bool kGuardCoordinates = true;

        /// Constructor
        /// \param emitter target emitter
        /// \param allocator shared texel allocator
        /// \param puidMemoryBaseBufferDataID buffer mapping PUIDs to memory offsets
        TexelPropertiesEmitter(E &emitter, const ComRef<TexelMemoryAllocator> &allocator, ID puidMemoryBaseBufferDataID) :
            emitter(emitter),
            allocator(allocator),
            puidMemoryBaseBufferDataID(puidMemoryBaseBufferDataID) {
            
        }

        /// Get the texel properties from an instruction
        /// \param instr instruction, must address a texel
        /// \return texel properties, always valid
        TexelProperties GetTexelProperties(const InstructionRef<> instr) {
            Program *program = emitter.GetProgram();

            // Get the resource being accessed
            ID resource = GetResource(instr.Get());

            // Get the buffer id
            // Stores all the texel blocks
            ID texelBlockBufferDataID = program->GetShaderDataMap().Get(allocator->GetTexelBlocksBufferID())->id;
            
            // Number of dimensions of the resource
            uint32_t dimensions = 1u;

            // Is this resource volumetric in nature? Affects address computation
            bool isVolumetric = false;

            // Determine from resource type
            const Type *resourceType = program->GetTypeMap().GetType(resource);
            if (auto texture = resourceType->Cast<TextureType>()) {
                dimensions = GetDimensionSize(texture->dimension);
                isVolumetric = texture->dimension == TextureDimension::Texture3D;
            } else if (auto buffer = resourceType->Cast<BufferType>()) {
                // Boo!
            } else {
                ASSERT(false, "Invalid type");
                return {};
            }

            // Set up the token emitter
            ResourceTokenEmitter token(emitter, resource);

            // Set up the properties
            TexelProperties out;
            out.puid = token.GetPUID();
            out.packedToken = token.GetPackedToken();

            // Defaults
            ID zero = program->GetConstants().UInt(0)->id;

            // Addressing offsets
            out.x = zero;
            out.y = zero;
            out.z = zero;
            out.mip = zero;
            out.offset = zero;

            // Get offsets from instruction
            switch (instr->opCode) {
                default: {
                    ASSERT(false, "Invalid instruction");
                    return {};
                }
                case OpCode::LoadBuffer: {
                    auto _instr = instr->As<LoadBufferInstruction>();
                    out.x = _instr->index;

                    if (_instr->offset != InvalidID) {
                        out.offset = _instr->offset;
                    }
                    break;
                }
                case OpCode::StoreBuffer: {
                    auto _instr = instr->As<StoreBufferInstruction>();
                    out.x = _instr->index;

                    if (_instr->offset != InvalidID) {
                        out.offset = _instr->offset;
                    }
                    break;
                }
                case OpCode::LoadBufferRaw: {
                    auto _instr = instr->As<LoadBufferRawInstruction>();
                    out.x = _instr->index;

                    if (_instr->offset != InvalidID) {
                        out.offset = _instr->offset;
                    }
                    break;
                }
                case OpCode::StoreBufferRaw: {
                    auto _instr = instr->As<StoreBufferRawInstruction>();
                    out.x = _instr->index;

                    if (_instr->offset != InvalidID) {
                        out.offset = _instr->offset;
                    }
                    break;
                }
                case OpCode::StoreTexture: {
                    auto _instr = *instr->As<StoreTextureInstruction>();

                    // Vectorized index?
                    if (const Type *indexType = program->GetTypeMap().GetType(_instr.index); indexType->Is<VectorType>()) {
                        if (dimensions > 0) out.x = emitter.Extract(_instr.index, program->GetConstants().UInt(0)->id);
                        if (dimensions > 1) out.y = emitter.Extract(_instr.index, program->GetConstants().UInt(1)->id);
                        if (dimensions > 2) out.z = emitter.Extract(_instr.index, program->GetConstants().UInt(2)->id);
                    } else {
                        out.x = _instr.index;
                    }
                    break;
                }
                case OpCode::LoadTexture: {
                    auto _instr = *instr->As<LoadTextureInstruction>();

                    // Vectorized index?
                    if (const Type *indexType = program->GetTypeMap().GetType(_instr.index); indexType->Is<VectorType>()) {
                        if (dimensions > 0) out.x = emitter.Extract(_instr.index, program->GetConstants().UInt(0)->id);
                        if (dimensions > 1) out.y = emitter.Extract(_instr.index, program->GetConstants().UInt(1)->id);
                        if (dimensions > 2) out.z = emitter.Extract(_instr.index, program->GetConstants().UInt(2)->id);
                    } else {
                        out.x = _instr.index;
                    }

                    if (_instr.mip != InvalidID) {
                        out.mip = _instr.mip;
                    }
                    break;
                }
                case OpCode::SampleTexture: {
                    // todo[init]: sampled offsets!
#if 0
                    auto _instr = instr->As<IL::SampleTextureInstruction>();

                    IL::ID coordinate = _instr->coordinate;
                    if (dimensions > 0) x = emitter.Mul(emitter.Extract(coordinate, program->GetConstants().UInt(0)->id);
                    if (dimensions > 1) y = emitter.Extract(coordinate, program->GetConstants().UInt(1)->id);
                    if (dimensions > 2) z = emitter.Extract(coordinate, program->GetConstants().UInt(2)->id);
                    
                    emitter.Mul(_instr->coordinate, )
#endif
                    break;
                }
            }

            // Get the base memory offset for the resource, points to the header
            ID resourceBaseMemoryOffset = emitter.Extract(emitter.LoadBuffer(emitter.Load(puidMemoryBaseBufferDataID), out.puid), zero);
            
            // Set up the subresource emitter
            SubresourceEmitter subresourceEmitter(emitter, token, emitter.Load(texelBlockBufferDataID), resourceBaseMemoryOffset);
            out.texelBaseOffsetAlign32 = subresourceEmitter.GetResourceMemoryBase();

            // Calculate the texel address
            // Different resource types may use different addressing schemas
            TexelAddressEmitter addressEmitter(emitter, token, subresourceEmitter);
            if (resourceType->Is<TextureType>()) {
                out.address = addressEmitter.LocalTextureTexelAddress(out.x, out.y, out.z, out.mip, isVolumetric);
            } else {
                ASSERT(resourceType->Is<Backend::IL::BufferType>(), "Expected buffer type");
                out.address = addressEmitter.LocalBufferTexelAddress(out.x, out.offset);
            }

            // OK
            return out;
        }

    private:
        /// Get the resource of an instruction
        /// \param instr must address a resource
        /// \return resource id
        ID GetResource(const Instruction* instr) {
            switch (instr->opCode) {
                default:
                    ASSERT(false, "Invalid instruction");
                    return InvalidID;
                case OpCode::LoadBuffer:
                    return instr->As<LoadBufferInstruction>()->buffer;
                case OpCode::StoreBuffer:
                    return instr->As<StoreBufferInstruction>()->buffer;
                case OpCode::LoadBufferRaw:
                    return instr->As<LoadBufferRawInstruction>()->buffer;
                case OpCode::StoreBufferRaw:
                    return instr->As<StoreBufferRawInstruction>()->buffer;
                case OpCode::StoreTexture:
                    return instr->As<StoreTextureInstruction>()->texture;
                case OpCode::LoadTexture:
                    return instr->As<LoadTextureInstruction>()->texture;
                case OpCode::SampleTexture:
                    return instr->As<SampleTextureInstruction>()->texture;
                
            }
        }

    private:
        /// Current emitter
        E &emitter;

        /// Shared texel allocator
        ComRef<TexelMemoryAllocator> allocator;

        /// Puid mapping buffer id
        ID puidMemoryBaseBufferDataID;
    };
}
