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

#include <ShaderExportGenerator.h>

#include <Backend/ShaderExport.h>
#include <sstream>
#include <iostream>

bool ShaderExportGenerator::Generate(Schema &schema, Language language, SchemaStream &out) {
    for (Message& message : schema.messages) {
        // Append shader guid if not disabled
        if (!message.attributes.GetBool("no-sguid")) {
            Field& sguid = *message.fields.emplace(message.fields.begin());
            sguid.name = "sguid";
            sguid.type = "uint16";

            // Attributes
            sguid.attributes.Add("bits", std::to_string(kShaderSGUIDBitCount));
        }
    }

    // Include emitter
    if (language == Language::CPP) {
        out.header << "#include <Backend/IL/Emitter.h>\n";
    }

    // OK
    return true;
}

bool ShaderExportGenerator::Generate(const Message &message, Language language, MessageStream &out) {
    switch (language) {
        case Language::CPP:
            return GenerateCPP(message, out);
        case Language::CS:
            return GenerateCS(message, out);
    }

    return false;
}

bool ShaderExportGenerator::GenerateCPP(const Message &message, MessageStream &out) {
    // Begin shader type
    out.types << "\tstruct ShaderExport {\n";

    // SGUID?
    bool noSGUID = message.attributes.GetBool("no-sguid");
    out.types << "\t\tstatic constexpr bool kNoSGUID = " << (noSGUID ? "true" : "false") << ";\n";

    // Structured?
    bool structured = message.attributes.GetBool("structured");
    out.types << "\t\tstatic constexpr bool kStructured = " << (structured ? "true" : "false") << ";\n\n";

    // Begin construction function
    out.types << "\t\ttemplate<typename OP>\n";
    out.types << "\t\tvoid Construct(IL::Emitter<OP>& emitter, uint32_t *dwordCount, IL::ID* dwords) const {\n";

    // Simple write?
    if (message.chunks.empty()) {
        // Size query?
        out.types << "\t\t\tif (!dwords) {\n";
        out.types << "\t\t\t\t*dwordCount = 1u;\n";
        out.types << "\t\t\t\treturn;\n";
        out.types << "\t\t\t}\n";

        // Create primary key
        out.types << "\t\t\tIL::ID primary = emitter.UInt(32, 0);\n";

        // Current offset
        uint32_t bitOffset = 0;

        // Append all fields
        for (const Field& field : message.fields) {
            auto it = primitiveTypeMap.types.find(field.type);
            if (it == primitiveTypeMap.types.end()) {
                std::cerr << "Malformed command in line: " << message.line << ", type " << field.type << " not supported for non structured writes" << std::endl;
                return false;
            }

            // Optional bit size
            auto bits = field.attributes.Get("bits");

            // Determine the size of this field
            uint32_t bitSize = bits ? std::atoi(bits->value.c_str()) : (static_cast<uint32_t>(it->second.size) * 8);

            // Bit masking
            uint32_t bitMask = static_cast<uint32_t>((1ull << bitSize) - 1u);

            // Append value
            out.types << "\t\t\tprimary = emitter.BitOr(primary, emitter.BitShiftLeft(emitter.BitAnd(" << field.name << ", emitter.UInt32(" << bitMask << ")), emitter.UInt(32, " << bitOffset << ")));\n";

            // Next
            bitOffset += bitSize;
        }

        // Check non structured write limit
        if (bitOffset > 32) {
            std::cerr << "Malformed command in line: " << message.line << ", non structured size exceeded 32 bits with " << bitOffset << " bits" << std::endl;
            return false;
        }

        // If chunked, just write out the primary as is
        out.types << "\t\t\tdwords[0] = primary;\n";
    } else if (!structured) {
        // Determine which chunks are of interest
        for (const Chunk& chunk : message.chunks) {
            out.types << "\t\t\tconst bool Append" << chunk.name << " = chunks.value & static_cast<uint32_t>(Chunk::" << chunk.name << ");\n";
        }

        // Size query?
        {
            out.types << "\n";
            out.types << "\t\t\tif (!dwords) {\n";
            out.types << "\t\t\t\t*dwordCount = 1u;\n";

            // Append each size based on visibility
            for (const Chunk& chunk : message.chunks) {
                out.types << "\t\t\t\t*dwordCount += " << chunk.name << "Chunk::kDWordCount * Append" << chunk.name << ";\n";
            }

            // Query end
            out.types << "\t\t\t\treturn;\n";
            out.types << "\t\t\t}\n";
        }

        // Append the chunk mask at the end of the primary key
        out.types << "\n";
        out.types << "\t\t\tconst uint32_t chunkMask = static_cast<uint32_t>(chunks.value) << (32u - static_cast<uint32_t>(Chunk::Count));\n";

        // Create primary key
        out.types << "\n";
        out.types << "\t\t\tuint32_t offset = 0;\n";
        out.types << "\t\t\tdwords[offset] = emitter.UInt(32, chunkMask);\n";

        // Current offset
        uint32_t bitOffset = 0;

        // Append all fields
        for (const Field& field : message.fields) {
            auto it = primitiveTypeMap.types.find(field.type);
            if (it == primitiveTypeMap.types.end()) {
                std::cerr << "Malformed command in line: " << message.line << ", type " << field.type << " not supported for non structured writes" << std::endl;
                return false;
            }

            // Optional bit size
            auto bits = field.attributes.Get("bits");

            // Determine the size of this field
            uint32_t bitSize = bits ? std::atoi(bits->value.c_str()) : (static_cast<uint32_t>(it->second.size) * 8);

            // Bit masking
            uint32_t bitMask = static_cast<uint32_t>((1ull << bitSize) - 1u);

            // Append value
            out.types << "\t\t\tdwords[offset] = emitter.BitOr(dwords[offset], emitter.BitShiftLeft(emitter.BitAnd(" << field.name << ", emitter.UInt32(" << bitMask << ")), emitter.UInt(32, " << bitOffset << ")));\n";

            // Next
            bitOffset += bitSize;
        }

        // Check non structured write limit
        if (bitOffset > 32) {
            std::cerr << "Malformed command in line: " << message.line << ", non structured size exceeded 32 bits with " << bitOffset << " bits" << std::endl;
            return false;
        }

        // Next!
        out.types << "\t\t\toffset++;\n";
        out.types << "\n";

        // Emit all dynamic chunks as needed
        for (const Chunk& chunk : message.chunks) {
            out.types << "\t\t\tif (Append" << chunk.name << ") {\n";

            // Lowercase name
            std::string scopeName = chunk.name;
            scopeName[0] = std::tolower(scopeName[0]);

            // Current bit offset
            bitOffset = 0;

            // Emit all fields
            for (const Field& field : chunk.fields) {
                // Next dword? (must be aligned)
                if (bitOffset == 32) {
                    bitOffset = 0;
                    out.types << "\t\t\t\toffset++;\n";
                }

                // Handle type
                if (auto it = primitiveTypeMap.types.find(field.type); it != primitiveTypeMap.types.end()) {
                    // Optional bit size
                    auto bits = field.attributes.Get("bits");

                    // Determine the size of this field
                    uint32_t bitSize = bits ? std::atoi(bits->value.c_str()) : (static_cast<uint32_t>(it->second.size) * 8);

                    if (bitOffset + bitSize > 32) {
                        std::cerr << "Malformed command in line: " << message.line << ", bit offsets must be dword aligned" << std::endl;
                        return false;
                    }

                    // Emit base value
                    if (bitOffset == 0) {
                        out.types << "\t\t\t\tdwords[offset] = emitter.UInt(32, 0);\n";
                    }

                    // Bit masking
                    uint32_t bitMask = static_cast<uint32_t>((1ull << bitSize) - 1u);

                    // Append value
                    out.types << "\t\t\t\tdwords[offset] = emitter.BitOr(dwords[offset], emitter.BitShiftLeft(emitter.BitAnd(" << scopeName << "." << field.name << ", emitter.UInt32(" << bitMask << ")), emitter.UInt(32, " << bitOffset << ")));\n";

                    // Next
                    bitOffset += bitSize;
                } else if (field.type == "array") {
                    if (bitOffset != 0) {
                        std::cerr << "Malformed command in line: " << message.line << ", chunk arrays must be dword aligned" << std::endl;
                        return false;
                    }

                    // Get the type
                    const Attribute *elementTypeName = field.attributes.Get("element");
                    if (!elementTypeName) {
                        std::cerr << "Malformed command in line: " << field.line << ", element type not found" << std::endl;
                        return false;
                    }

                    // Get the inbuilt type
                    auto elementIt = primitiveTypeMap.types.find(elementTypeName->value);
                    if (elementIt == primitiveTypeMap.types.end()) {
                        std::cerr << "Malformed command in line: " << field.line << ", unknown array type '" << elementTypeName->value << "'" << std::endl;
                        return false;
                    }

                    // Get the length
                    const Attribute *length = field.attributes.Get("length");
                    if (!length) {
                        std::cerr << "Malformed command in line: " << field.line << ", length not found" << std::endl;
                        return false;
                    }

                    // Parse length
                    const uint32_t lengthAttribute = std::atoi(length->value.c_str());

                    // Simply write the dword values
                    for (uint32_t i = 0; i < lengthAttribute; i++) {
                        out.types << "\t\t\t\tdwords[offset] = " << scopeName << "." << field.name << "[" << i << "];\n";
                        out.types << "\t\t\t\toffset++;\n";
                    }
                } else {
                    std::cerr << "Malformed command in line: " << message.line << ", type " << field.type << " not supported for chunk writes" << std::endl;
                    return false;
                }
            }

            // Next!
            out.types << "\t\t\t}\n\n";
        }

        // Validation
        out.types << "\t\t\tASSERT(offset <= *dwordCount, \"Append out of bounds\");\n";
    } else {
        // Soon (tm)
        std::cerr << "Malformed command in line: " << message.line << ", structured writes not supported yet" << std::endl;
        return false;
    }

    // End construction function
    out.types << "\t\t}\n\n";

    // User fed chunk mask
    if (!message.chunks.empty()) {
        out.types << "\t\tChunkSet chunks{0};\n";
    }

    // Begin shader values
    for (const Field& field : message.fields) {
        out.types << "\t\tIL::ID " << field.name << "{IL::InvalidID};\n";
    }

    // Chunk data, each wrapped in an anonymous structure
    for (const Chunk& chunk : message.chunks) {
        out.types << "\t\tstruct {\n";

        // Emit each field
        for (const Field &field: chunk.fields) {
            // Handle type
            if (primitiveTypeMap.types.find(field.type) != primitiveTypeMap.types.end()) {
                // Emit as singular
                out.types << "\t\t\tIL::ID " << field.name << "{IL::InvalidID};\n";
            } else if (field.type ==  "array") {
                // Get the length
                const Attribute *length = field.attributes.Get("length");
                if (!length) {
                    std::cerr << "Malformed command in line: " << field.line << ", length not found" << std::endl;
                    return false;
                }

                // Emit as array
                const uint32_t lengthAttribute = std::atoi(length->value.c_str());
                out.types << "\t\t\tIL::ID " << field.name << "[" << lengthAttribute << "];\n";
            } else {
                std::cerr << "Malformed command in line: " << message.line << ", type " << field.type << " not supported for chunk writes" << std::endl;
                return false;
            }
        }

        // Lowercase
        std::string scopeName = chunk.name;
        scopeName[0] = std::tolower(scopeName[0]);

        // Emit as named
        out.types << "\t\t} " << scopeName << ";\n";
    }

    // End shader type
    out.types << "\t};\n\n";

    // Add caster and creator for non structured types
    if (!structured) {
        out.types << "\tuint32_t GetKey() const {\n";
        out.types << "\t\tunion {\n";
        out.types << "\t\t\tuint32_t key;\n";
        out.types << "\t\t\t" << message.name << "Message message;\n";
        out.types << "\t\t} u = {.message = *this};\n";
        out.types << "\t\treturn u.key;\n";
        out.types << "\t}\n";

        out.types << "\tstatic " << message.name << "Message FromKey(uint32_t key) {\n";
        out.types << "\t\tunion {\n";
        out.types << "\t\t\tuint32_t key;\n";
        out.types << "\t\t\t" << message.name << "Message message;\n";
        out.types << "\t\t} u = {.key = key};\n";
        out.types << "\t\treturn u.message;\n";
        out.types << "\t}\n";
    }

    // OK
    return true;
}

bool ShaderExportGenerator::GenerateCS(const Message &message, MessageStream &out) {
    // Structured?
    bool structured = message.attributes.GetBool("structured");
    out.types << "\t\tpublic const bool IsStructured = " << (structured ? "true" : "false") << ";\n\n";

    // Add getter and setter for non-structured types (i.e. single uint)
    if (!structured) {
        out.types << "\t\tpublic uint Key\n";
        out.types << "\t\t{\n";
        out.types << "\t\t\t[MethodImpl(MethodImplOptions.AggressiveInlining)]\n";
        out.types << "\t\t\tget\n";
        out.types << "\t\t\t{\n";

        if (message.chunks.empty()) {
            out.types << "\t\t\t\tuint key = MemoryMarshal.Read<uint>(_memory.Slice(0, 4).AsRefSpan());\n";
        } else {
            out.types << "\t\t\t\tuint key = _primary;\n";
            out.types << "\t\t\t\tkey &= ~((int)Chunk.Mask << (32 - (int)Chunk.Count));\n";
        }
        
        out.types << "\t\t\t\treturn key;\n";
        out.types << "\t\t\t}\n";
        out.types << "\t\t}\n";
    }

    return true;
}
