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

// Backend
#include <Backend/IL/Type.h>
#include <Backend/IL/Tiny/TinyType.h>

// Std
#include <vector>

namespace Backend::IL::Tiny {
    namespace Detail {
        template<typename T>
        void PackAppend(const T &type, std::vector<uint8_t> &buffer) {
            buffer.insert(
                buffer.end(),
                reinterpret_cast<const uint8_t *>(&type),
                reinterpret_cast<const uint8_t *>(&type + 1)
            );
        }

        /// Pack a type into a tiny type
        /// @param type type to pack
        /// @param tinyID starting id
        /// @param buffer destination buffer
        static void Pack(const IL::Type *type, RelativeTinyID& tinyID, std::vector<uint8_t> &buffer) {
            // Self
            tinyID++;
            
            switch (type->kind) {
                case TypeKind::None: {
                    ASSERT(false, "Invalid type");
                    break;
                }
                case TypeKind::Bool: {
                    Type packed;
                    packed.kind = TypeKind::Bool;
                    PackAppend(packed, buffer);
                    break;
                }
                case TypeKind::Void: {
                    Type packed;
                    packed.kind = TypeKind::Void;
                    PackAppend(packed, buffer);
                    break;
                }
                case TypeKind::Int: {
                    auto *typed = type->As<IL::IntType>();

                    IntType packed;
                    packed.kind = TypeKind::Int;
                    packed.bitWidth = typed->bitWidth;
                    packed.signedness = typed->signedness;
                    PackAppend(packed, buffer);
                    break;
                }
                case TypeKind::FP: {
                    auto *typed = type->As<IL::FPType>();

                    FPType packed;
                    packed.kind = TypeKind::FP;
                    packed.bitWidth = typed->bitWidth;
                    PackAppend(packed, buffer);
                    break;
                }
                case TypeKind::Vector: {
                    auto *typed = type->As<IL::VectorType>();

                    VectorType packed;
                    packed.kind = TypeKind::Vector;
                    packed.containedType = tinyID;
                    packed.dimension = typed->dimension;
                    PackAppend(packed, buffer);

                    Pack(typed->containedType, tinyID, buffer);
                    break;
                }
                case TypeKind::Matrix: {
                    auto *typed = type->As<IL::MatrixType>();

                    MatrixType packed;
                    packed.kind = TypeKind::Matrix;
                    packed.containedType = tinyID;
                    packed.rows = typed->rows;
                    packed.columns = typed->columns;
                    PackAppend(packed, buffer);

                    Pack(typed->containedType, tinyID, buffer);
                    break;
                }
                case TypeKind::Pointer: {
                    auto *typed = type->As<IL::PointerType>();

                    PointerType packed;
                    packed.kind = TypeKind::Pointer;
                    packed.pointee = tinyID;
                    packed.addressSpace = typed->addressSpace;
                    PackAppend(packed, buffer);

                    Pack(typed->pointee, tinyID, buffer);
                    break;
                }
                case TypeKind::Array: {
                    auto *typed = type->As<IL::ArrayType>();

                    ArrayType packed;
                    packed.kind = TypeKind::Array;
                    packed.elementType = tinyID;
                    packed.count = typed->count;
                    PackAppend(packed, buffer);

                    Pack(typed->elementType, tinyID, buffer);
                    break;
                }
                case TypeKind::Texture: {
                    auto *typed = type->As<IL::TextureType>();

                    TextureType packed;
                    packed.kind = TypeKind::Texture;
                    packed.dimension = typed->dimension;
                    packed.format = typed->format;
                    packed.multisampled = typed->multisampled;
                    packed.samplerMode = typed->samplerMode;
                    packed.sampledType = tinyID;
                    PackAppend(packed, buffer);

                    Pack(typed->sampledType, tinyID, buffer);
                    break;
                }
                case TypeKind::Buffer: {
                    auto *typed = type->As<IL::BufferType>();

                    BufferType packed;
                    packed.kind = TypeKind::Texture;
                    packed.samplerMode = typed->samplerMode;
                    packed.byteAddressing = typed->byteAddressing;
                    packed.texelType = typed->texelType;
                    packed.elementType = tinyID;
                    PackAppend(packed, buffer);

                    Pack(typed->elementType, tinyID, buffer);
                    break;
                }
                case TypeKind::Sampler: {
                    Type packed;
                    packed.kind = TypeKind::Sampler;
                    PackAppend(packed, buffer);
                    break;
                }
                case TypeKind::CBuffer: {
                    Type packed;
                    packed.kind = TypeKind::CBuffer;
                    PackAppend(packed, buffer);
                    break;
                }
                case TypeKind::Function: {
                    auto *typed = type->As<IL::FunctionType>();

                    FunctionType packed;
                    packed.kind = TypeKind::Function;
                    packed.returnType = tinyID;
                    packed.parameterTypes.count = static_cast<uint32_t>(typed->parameterTypes.size());
                    PackAppend(packed, buffer);

                    uint64_t typeOffset = buffer.size();
                    buffer.resize(buffer.size() + sizeof(RelativeTinyID) * packed.parameterTypes.count);

                    Pack(typed->returnType, tinyID, buffer);

                    for (uint64_t i = 0; i < packed.parameterTypes.count; i++) {
                        reinterpret_cast<RelativeTinyID*>(buffer.data() + typeOffset)[i] = tinyID;
                        Pack(typed->parameterTypes[i], tinyID, buffer);
                    } 
                    break;
                }
                case TypeKind::Struct: {
                    auto *typed = type->As<IL::StructType>();

                    StructType packed;
                    packed.kind = TypeKind::Struct;
                    packed.memberTypes.count = static_cast<uint32_t>(typed->memberTypes.size());
                    PackAppend(packed, buffer);

                    uint64_t typeOffset = buffer.size();
                    buffer.resize(buffer.size() + sizeof(RelativeTinyID) * packed.memberTypes.count);

                    for (uint64_t i = 0; i < packed.memberTypes.count; i++) {
                        reinterpret_cast<RelativeTinyID*>(buffer.data() + typeOffset)[i] = tinyID;
                        Pack(typed->memberTypes[i], tinyID, buffer);
                    } 
                    break;
                }
                case TypeKind::Unexposed: {
                    Type packed;
                    packed.kind = TypeKind::Unexposed;
                    PackAppend(packed, buffer);
                    break;
                }
            }
        }
    }

    /// Pack a type into a tiny type
    /// @param type type to pack
    /// @param buffer destination buffer
    static void Pack(const IL::Type *type, std::vector<uint8_t> &buffer) {
        RelativeTinyID tinyID = 0;
        return Detail::Pack(type, tinyID, buffer);
    }
}
