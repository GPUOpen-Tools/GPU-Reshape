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
#include <Backend/IL/InstructionAddressCommon.h>
#include <Backend/IL/TypeSize.h>
#include <Backend/IL/Analysis/StructuralUserAnalysis.h>
#include <Backend/IL/Metadata/DataMetadata.h>
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
            structuralUserAnalysis = emitter.GetProgram()->GetAnalysisMap().template FindPass<StructuralUserAnalysis>();
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
            out.texelCountLiteral = 1u;
            
            // Number of dimensions of the resource
            uint32_t dimensions = 1u;

            // Is this resource volumetric in nature? Affects address computation
            bool isVolumetric = false;

            // Get the resource type
            const Type *resourceType = program->GetTypeMap().GetType(resource);

            // Certain operations, like load / stores, may operate with the variables themselves
            // Unwrap the type
            if (auto pointerType = resourceType->Cast<PointerType>()) {
                resourceType = pointerType->pointee;
            }

            // Determine from resource type
            if (auto texture = resourceType->Cast<TextureType>()) {
                dimensions = GetDimensionSize(texture->dimension);
                isVolumetric = texture->dimension == TextureDimension::Texture3D;
            } else if (auto buffer = resourceType->Cast<BufferType>()) {
                // Poof
            } else {
                ASSERT(false, "Invalid type");
                return {};
            }

            // Is this a plain memory address?
            bool isMemoryAddressing = false;

            // Get offsets from instruction
            switch (instr->opCode) {
                default: {
                    ASSERT(false, "Invalid instruction");
                    return {};
                }
                case OpCode::LoadBuffer: {
                    auto _instr = instr->As<LoadBufferInstruction>();
                    out.x = _instr->index;

                    // Get the result type
                    const Type* type = program->GetTypeMap().GetType(_instr->result);

                    // Buffer load instructions always return a 4 component float
                    // To figure out what we're <actually> using, extract a component mask from the structural users
                    ComponentMask mask = structuralUserAnalysis->GetUsedComponentMask(_instr->result);

                    // Get the component type
                    const Type* componentType = GetStructuralType(type, 0);

                    // Determine the effective byte range from the structural mask
                    out.texelCountLiteral = static_cast<uint32_t>(GetTypeByteSize(*program, componentType) * std::popcount(static_cast<uint32_t>(mask)));

                    if (_instr->offset != InvalidID) {
                        out.offset = _instr->offset;
                    }
                    break;
                }
                case OpCode::StoreBuffer: {
                    auto _instr = instr->As<StoreBufferInstruction>();
                    out.x = _instr->index;

                    // Get the buffer
                    auto buffer = resourceType->As<BufferType>();

                    // Number of bytes per component
                    size_t componentSize;

                    // Number of dimensions
                    uint32_t dimensionCount;

                    // If a type is specified, this is not a format-based storage
                    if (buffer->elementType) {
                        auto resultType = program->GetTypeMap().GetType(_instr->value);
                        dimensionCount = GetTypeDimension(resultType);
                        componentSize = GetTypeByteSize(*program, GetComponentType(resultType));
                    } else {
                        dimensionCount = GetDimensionSize(buffer->texelType);
                        componentSize = GetSize(buffer->texelType) / dimensionCount;
                    }

                    // Determine the effective byte range from the specified store mask
                    out.texelCountLiteral = static_cast<uint32_t>(componentSize * std::min(dimensionCount, static_cast<uint32_t>(std::popcount(_instr->mask.value))));

                    if (_instr->offset != InvalidID) {
                        out.offset = _instr->offset;
                    }
                    break;
                }
                case OpCode::LoadBufferRaw: {
                    auto _instr = instr->As<LoadBufferRawInstruction>();
                    
                    // Get the buffer
                    auto buffer = resourceType->As<BufferType>();

                    // If byte addressing, the index is the actual byte offset
                    if (buffer->byteAddressing) {
                        isMemoryAddressing = true;
                        out.texelCountLiteral = 1u;
                        out.offset = _instr->index;
                        ASSERT(_instr->offset == InvalidID, "Byte addressing only expects an index");
                    } else {
                        out.x = _instr->index;
                        out.texelCountLiteral = 1u;

                        // Intra-element offset is optional
                        if (_instr->offset != InvalidID) {
                            out.offset = _instr->offset;
                        }
                    }

                    break;
                }
                case OpCode::StoreBufferRaw: {
                    auto _instr = instr->As<StoreBufferRawInstruction>();
                    
                    // Get the buffer
                    auto buffer = resourceType->As<BufferType>();
                    
                    // If byte addressing, the index is the actual byte offset
                    if (buffer->byteAddressing) {
                        isMemoryAddressing = true;
                        out.texelCountLiteral = 1u;
                        out.offset = _instr->index;
                        ASSERT(_instr->offset == InvalidID, "Byte addressing only expects an index");
                    } else {
                        out.x = _instr->index;
                        out.texelCountLiteral = 1u;
                    
                        // Intra-element offset is optional
                        if (_instr->offset != InvalidID) {
                            out.offset = _instr->offset;
                        }
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
                case IL::OpCode::Load: {
                    auto _instr = instr->As<IL::LoadInstruction>();

                    // Validate type
                    auto type = program->GetTypeMap().GetType(_instr->address)->As<Backend::IL::PointerType>();
                    ASSERT(IsGenericResourceAddressSpace(type), "Invalid load instruction");

                    // Get the coordinate and offset from the access chain
                    TraverseAccessChainIndexing(_instr->address, out.x, out.offset, out.texelCountLiteral);
                    isMemoryAddressing = true;
                    break;
                }
                case IL::OpCode::Store: {
                    auto _instr = instr->As<IL::StoreInstruction>();
                    
                    // Validate type
                    auto type = program->GetTypeMap().GetType(_instr->address)->As<Backend::IL::PointerType>();
                    ASSERT(IsGenericResourceAddressSpace(type), "Invalid load instruction");

                    // Get the coordinate and offset from the access chain
                    TraverseAccessChainIndexing(_instr->address, out.x, out.offset, out.texelCountLiteral);
                    isMemoryAddressing = true;
                    break;
                }
            }

            // If the program is sign-less, assume all coordinates are unsigned
            if (!program->GetCapabilityTable().integerSignIsUnique) {
                auto* uint32 = program->GetTypeMap().FindTypeOrAdd(IntType { .bitWidth = 32, .signedness = false });
                if (out.x != zero) out.x = emitter.BitCast(out.x, uint32);
                if (out.y != zero) out.y = emitter.BitCast(out.y, uint32);
                if (out.z != zero) out.z = emitter.BitCast(out.z, uint32);
                if (out.mip != zero) out.mip = emitter.BitCast(out.mip, uint32);
                if (out.offset != zero) out.offset = emitter.BitCast(out.offset, uint32);
            }

            // Get the base memory offset for the resource, points to the header
            ID resourceBaseMemoryOffset = emitter.Extract(emitter.LoadBuffer(emitter.Load(puidMemoryBaseBufferDataID), out.puid), zero);
            
            // Set up the subresource emitter
            InlineSubresourceEmitter subresourceEmitter(emitter, token, emitter.Load(texelBlockBufferDataID), resourceBaseMemoryOffset);
            out.texelBaseOffsetAlign32 = subresourceEmitter.GetResourceMemoryBase();

            // Calculate the texel address
            // Different resource types may use different addressing schemas
            TexelAddressEmitter addressEmitter(emitter, token, subresourceEmitter);
            if (isMemoryAddressing) {
                ASSERT(resourceType->Is<Backend::IL::BufferType>(), "Expected buffer type");
                out.address = addressEmitter.LocalMemoryTexelAddress(out.x, out.offset, out.texelCountLiteral);
            } else if (resourceType->Is<TextureType>()) {
                out.address = addressEmitter.LocalTextureTexelAddress(out.x, out.y, out.z, out.mip, isVolumetric);
            } else if (resourceType->Is<BufferType>()) {
                out.address = addressEmitter.LocalBufferTexelAddress(out.x, out.offset, out.texelCountLiteral);
            } else {
                ASSERT(false, "Invalid type");
            }

            // Get failure condition
            out.failureBlock = subresourceEmitter.ReadFieldDWord(TexelMemoryDWordFields::FailureBlock);

            // OK
            return out;
        }

    private:
        // todo[init]: move out
        struct ByteOffsetAccumulator {
            ByteOffsetAccumulator(E& emitter) : emitter(emitter) {
                
            }

            /// Add a literal
            /// \param value literal value to add 
            void AddLiteral(uint32_t value) {
                if (id != InvalidID) {
                    id = emitter.Add(id, emitter.UInt32(value));
                } else {
                    literal += value;
                }
            }
            
            /// Add an id
            /// \param value id value to add
            void AddID(IL::ID value) {
                if (id == InvalidID) {
                    id = emitter.UInt32(literal);
                }

                id = emitter.Add(id, value);
            }

            /// Get the id, creates it if not present
            /// \return id
            IL::ID GetID() {
                if (id == InvalidID) {
                    return emitter.UInt32(literal);
                }

                return id;
            }
            
            /// Current emitter
            E &emitter;

            /// Current literal value
            uint32_t literal{0};

            /// Current id value
            IL::ID id{InvalidID};
        };

        /// Traverse an access chain and determine its texel properties
        /// \param address address to traverse
        /// \param x output base coordinate
        /// \param byteOffset output byte offset
        /// \param byteCountLiteral output compile time byte count
        void TraverseAccessChainIndexing(IL::ID address, IL::ID& x, IL::ID& byteOffset, uint32_t& byteCountLiteral) {
            Program *program = emitter.GetProgram();

            // Resource value type
            const Type* valueType{nullptr};

            // Walk back the access chain
            TrivialStackVector<IL::ID, 8u> chain;
            IL::VisitGlobalAddressChainReverse(*program, address, [&](ID id, bool isCompositeBase) {
                // If we've reached the resource, stop
                // The resource may itself be fetched from some structure, but that doesn't matter
                if ((valueType = GetChainResourceValueType(id))) {
                    return false;
                }

                // Ignore composite bases
                if (isCompositeBase) {
                    return true;
                }

                chain.Add(id);
                return true;
            });

            // Reverse in-place
            std::ranges::reverse(chain);

            // Start at offset 0
            // Add them in IL, as the actual offset may end up being dynamic (e.g., arrays)
            ByteOffsetAccumulator accumulator(emitter);

            // Traverse chain stack, skip composite and base offset
            for (uint32_t i = 0; i < chain.Size(); i++) {
                // If a struct type, the index must be constant
                if (auto structType = valueType->Cast<StructType>()) {
                    auto index = static_cast<uint32_t>(program->GetConstants().GetConstant(chain[i])->As<IntConstant>()->value);

                    // Set the next type
                    valueType = structType->memberTypes[index];
                    
                    // If the member has an offset, use that, if not, we need to compute the offsets ourselves
                    if (auto offset = program->GetMetadataMap().GetMetadata<OffsetMetadata>(structType->id, index)) {
                        accumulator.AddLiteral(offset->byteOffset);
                        continue;
                    }

                    // Accumulate all the offsets up until this member
                    for (uint32_t memberIndex = 0; memberIndex < index; memberIndex++) {
                        accumulator.AddLiteral(static_cast<uint32_t>(GetPODNonAlignedTypeByteSize(structType->memberTypes[memberIndex])));
                    }
                    
                    // Next!
                    continue;
                }

                const Type *containedType = GetComponentType(valueType);

                // If not structural, then assume some indexable type with constant strides (like arrays)
                uint32_t arrayByteStride;
                if (auto stride = program->GetMetadataMap().GetMetadata<ArrayStrideMetadata>(valueType->id)) {
                    arrayByteStride = stride->byteStride;
                } else {
                    arrayByteStride = static_cast<uint32_t>(GetPODNonAlignedTypeByteSize(containedType));
                }

                // If constant, inline the offsets, don't push to IL just yet
                if (const Constant *constant = program->GetConstants().GetConstant(chain[i])) {
                    accumulator.AddLiteral(arrayByteStride * static_cast<uint32_t>(constant->As<IntConstant>()->value));
                } else {
                    accumulator.AddID(emitter.Mul(emitter.UInt32(arrayByteStride), chain[i]));
                }

                // Set the next type
                valueType = containedType;
            }

            byteCountLiteral = static_cast<uint32_t>(GetTypeByteSize(*program, valueType));

            // Get or create the offset
            byteOffset = accumulator.GetID();
        }

        /// Get the value type of a resource
        /// \param id top id
        /// \return type
        const Type* GetChainResourceValueType(IL::ID id) {
            Program *program = emitter.GetProgram();

            // Try to get id
            auto type = program->GetTypeMap().GetType(id);
            if (!type) {
                return nullptr;
            }

            // Try to unwrap as pointer
            auto ptrType = type->Cast<PointerType>();
            if (!ptrType) {
                return nullptr;
            }

            // Get value type
            switch (ptrType->pointee->kind) {
                default: {
                    return nullptr;
                }
                case TypeKind::Buffer: {
                    return ptrType->pointee->As<BufferType>()->elementType;
                }
                case TypeKind::Texture: {
                    return ptrType->pointee->As<TextureType>()->sampledType;
                }
            }
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
                case OpCode::Load:
                    return GetResourceFromAccessChain(instr->As<LoadInstruction>()->address);
                case OpCode::Store:
                    return GetResourceFromAccessChain(instr->As<StoreInstruction>()->address);
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

        /// Get the resource from an access chain
        /// \param address given address
        /// \return resource id
        ID GetResourceFromAccessChain(IL::ID address) {
            Program *program = emitter.GetProgram();

            // Traverse back until we find the resource
            ID resourceId{InvalidID};
            IL::VisitGlobalAddressChainReverse(*program, address, [&](ID id, bool) {
                if (IsPointerToResourceType(program->GetTypeMap().GetType(id))) {
                    resourceId = id;
                    return false;
                }

                // Next!
                return true;
            });

            // Must have found a resource at this point
            ASSERT(resourceId != InvalidID, "Failed to find resource from access chain");
            return resourceId;
        }

    private:
        /// Current emitter
        E &emitter;

        /// Shared texel allocator
        ComRef<TexelMemoryAllocator> allocator;

        /// Structural analysis used for component tracking
        ComRef<StructuralUserAnalysis> structuralUserAnalysis;

        /// Puid mapping buffer id
        ID puidMemoryBaseBufferDataID;
    };
}
