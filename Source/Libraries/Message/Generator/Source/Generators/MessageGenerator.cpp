#include <Generators/MessageGenerator.h>

#include <sstream>
#include <iostream>

bool MessageGenerator::Generate(Schema &schema, Language language, SchemaStream &out) {
    return true;
}

bool MessageGenerator::Generate(const Message &message, Language language, MessageStream &out) {
    switch (language) {
        case Language::CPP:
            return GenerateCPP(message, out);
        case Language::CS:
            return GenerateCS(message, out);
    }

    return false;
}

bool MessageGenerator::GenerateCPP(const Message &message, MessageStream &out) {
    // Members of the message
    std::stringstream patch;
    std::stringstream byteSize;
    std::stringstream allocationParameters;

    // Any dynamic parameters?
    bool anyDynamic = false;

    // Total size
    uint64_t cxxSizeType = 0;

    // Current bit field type
    TypeInfo bitFieldType;

    // Current bit field offset
    uint32_t bitFieldOffset{0};

    // Generate all out.members
    for (auto fieldIt = message.fields.begin(); fieldIt != message.fields.end(); fieldIt++) {
        const Field &field = *fieldIt;

        // Get the default value
        const Attribute* defaultValue = field.attributes.Get("value");

        // Bit specifier?
        const Attribute* bits = field.attributes.Get("bits");
        if (bits) {
            auto typeIt = primitiveTypeMap.types.find(field.type);
            if (typeIt == primitiveTypeMap.types.end()) {
                std::cerr << "Malformed command in line: " << message.line << ", type '" << field.type << "' does not support bit attribute" << std::endl;
                return false;
            }

            // New bit field?
            if (!bitFieldType.size) {
                bitFieldType = typeIt->second;
                bitFieldOffset = 0;

                for (auto siblingIt = std::next(fieldIt); siblingIt != message.fields.end(); siblingIt++) {
                    const Attribute* siblingBits = siblingIt->attributes.Get("bits");
                    if (!siblingBits) {
                        break;
                    }

                    auto siblingTypeIt = primitiveTypeMap.types.find(siblingIt->type);
                    if (siblingTypeIt == primitiveTypeMap.types.end()) {
                        std::cerr << "Malformed command in line: " << message.line << ", type '" << siblingIt->type << "' does not support bit attribute" << std::endl;
                        return false;
                    }

                    if (siblingTypeIt->second.size > bitFieldType.size) {
                        bitFieldType = siblingTypeIt->second;
                    }
                }
            }
        } else {
            bitFieldType = {};
            bitFieldOffset = 0;
        }

        // Primitive?
        if (auto it = primitiveTypeMap.types.find(field.type); it != primitiveTypeMap.types.end()) {
            if (bits) {
                int32_t bitCount = std::atoi(bits->value.c_str());

                const uint32_t bitSize = static_cast<uint32_t>(bitFieldType.size) * 8;

                if (!bitFieldOffset || bitFieldOffset % bitSize == 0) {
                    cxxSizeType += bitFieldType.size;
                    byteSize << "\t\t\tsize += " << bitFieldType.size << ";\n";
                }

                out.members << "\t" << bitFieldType.cxxType << " " << field.name << " : " << bitCount << ";\n";

                const uint32_t bitElementBefore = static_cast<uint32_t>(bitFieldOffset / bitFieldType.size);
                const uint32_t bitElementAfter = static_cast<uint32_t>(bitFieldOffset / bitFieldType.size);

                if (bitElementAfter > bitElementBefore && (bitFieldOffset && bitFieldOffset % bitSize != 0)) {
                    std::cerr << "Malformed command in line: " << field.line << ", bit field size exceeded type size of " << bitFieldType.size << std::endl;
                    return false;
                }

                bitFieldOffset += bitCount;
            } else {
                cxxSizeType += it->second.size;

                byteSize << "\t\t\tsize += " << it->second.size << ";\n";

                out.members << "\t" << it->second.cxxType << " " << field.name;

                if (defaultValue) {
                    out.members << " = " << defaultValue->value;
                }

                out.members << ";\n";
            }
        } else if (field.type ==  "array") {
            // Get the type
            const Attribute* elementTypeName = field.attributes.Get("element");
            if (!elementTypeName) {
                std::cerr << "Malformed command in line: " << field.line << ", element type not found" << std::endl;
                return false;
            }

            // Get the inbuilt type
            if (it = primitiveTypeMap.types.find(elementTypeName->value); it == primitiveTypeMap.types.end()) {
                std::cerr << "Malformed command in line: " << field.line << ", unknown type '" << elementTypeName->value << "'" << std::endl;
                return false;
            }

            // Add allocation parameter
            allocationParameters << "\t\tsize_t " << field.name << "Count = 0;\n";

            // Add byte size
            byteSize << "\t\t\tsize += 16 + " << it->second.size << " * " << field.name << "Count" << ";\n";

            // Add patch
            patch << "\t\t\tmessage->" << field.name << ".count = " << field.name << "Count;\n";
            patch << "\t\t\tmessage->" << field.name << ".thisOffset = offset + sizeof(" << message.name << "Message) - offsetof(" << message.name << "Message, " << field.name << ");\n";
            patch << "\t\t\toffset += " << field.name << "Count * " << it->second.size << "; \n\n";

            // Requires the dynamic schema
            anyDynamic = true;

            // Append field
            out.members << "\tMessageArray<" << it->second.cxxType << "> " << field.name << ";\n";

            // Size type, not allocation type
            cxxSizeType += 16;
        } else if (field.type ==  "string") {
            // Add allocation parameter
            allocationParameters << "\t\tsize_t " << field.name << "Length = 0;\n";

            // Add byte size
            byteSize << "\t\t\tsize += 16 + sizeof(char) * " << field.name << "Length" << ";\n";

            // Add patch
            patch << "\t\t\tmessage->" << field.name << ".data.count = " << field.name << "Length;\n";
            patch << "\t\t\tmessage->" << field.name << ".data.thisOffset = offset + sizeof(" << message.name << "Message) - offsetof(" << message.name << "Message, " << field.name << ");\n";
            patch << "\t\t\toffset += " << field.name << "Length * sizeof(char); \n\n";

            // Requires the dynamic schema
            anyDynamic = true;

            // Append field
            out.members << "\tMessageString " << field.name << ";\n";

            // Size type, not allocation type
            cxxSizeType += 16;
        } else if (field.type ==  "stream") {
            // Add allocation parameter
            allocationParameters << "\t\tsize_t " << field.name << "ByteSize = 0;\n";

            // Add byte size
            byteSize << "\t\t\tsize += 32 + sizeof(uint8_t) * " << field.name << "ByteSize" << ";\n";

            // Add patch
            patch << "\t\t\tmessage->" << field.name << ".data.count = " << field.name << "ByteSize;\n";
            patch << "\t\t\tmessage->"
            << field.name << ".data.thisOffset = offset"
            << " + sizeof(" << message.name << "Message)"
            << " - offsetof(" << message.name << "Message, " << field.name << ")"
            << " - offsetof(MessageSubStream, data);\n";
            patch << "\t\t\toffset += " << field.name << "ByteSize * sizeof(uint8_t); \n\n";

            // Requires the dynamic schema
            anyDynamic = true;

            // Append field
            out.members << "\tMessageSubStream " << field.name << ";\n";

            // Size type, not allocation type
            cxxSizeType += 32;
        } else if (auto it = declaredTypes.find(field.type); it != declaredTypes.end()) {
            cxxSizeType += it->second.size;

            // Increase size
            byteSize << "\t\t\tsize += " << it->second.size << ";\n";

            // Add field
            out.members << "\t" << it->first << " " << field.name << ";\n";
        } else {
            std::cerr << "Malformed command in line: " << message.line << ", unknown type '" << field.type << "'" << std::endl;
            return false;
        }
    }

    // Append to type map
    TypeMeta meta;
    meta.size = cxxSizeType;
    declaredTypes[message.name] = meta;

    // Set schema
    if (anyDynamic) {
        out.schemaType << "DynamicMessageSchema";
    } else {
        out.schemaType << "StaticMessageSchema";
    }

    // Begin allocation info
    out.types << "\n";
    out.types << "\tstruct AllocationInfo {\n";

    // Byte size information
    out.types << "\t\t[[nodiscard]]\n";
    out.types << "\t\tuint64_t ByteSize() const {\n";
    out.types << "\t\t\tuint64_t size = 0;\n";
    out.types << byteSize.str();
    out.types << "\t\t\treturn size;\n";
    out.types << "\t\t}\n";

    // Allocation patching
    out.types << "\n";
    out.types << "\t\tvoid Patch(" << message.name << "Message* message) const {\n";
    out.types << "\t\t\tuint64_t offset = 0;\n";
    out.types << patch.str();
    out.types << "\t\t}";

    // Allocation parameters
    out.types << "\n\n";
    out.types << allocationParameters.str();

    // End allocation info
    out.types << "\t};\n";

    // Size check
    out.schema.footer << "static_assert(sizeof(" << message.name << "Message) == " << cxxSizeType << ", \"Unexpected compiler packing\");\n";

    // OK
    return true;
}

bool MessageGenerator::GenerateCS(const Message &message, MessageStream &out) {
    // Members of the message
    std::stringstream patch;
    std::stringstream byteSize;
    std::stringstream allocationParameters;

    // Any dynamic parameters?
    bool anyDynamic = false;

    // Total size
    uint64_t cxxSizeType = 0;

    // Current bit field type
    TypeInfo bitFieldType;

    // Current bit field offset
    uint32_t bitFieldOffset{0};

    // Generate all out.members
    for (auto fieldIt = message.fields.begin(); fieldIt != message.fields.end(); fieldIt++) {
        const Field &field = *fieldIt;

        // Get the default value
        const Attribute* defaultValue = field.attributes.Get("value");

        // Bit specifier?
        const Attribute* bits = field.attributes.Get("bits");
        if (bits) {
            auto typeIt = primitiveTypeMap.types.find(field.type);
            if (typeIt == primitiveTypeMap.types.end()) {
                std::cerr << "Malformed command in line: " << message.line << ", type '" << field.type << "' does not support bit attribute" << std::endl;
                return false;
            }

            // New bit field?
            if (!bitFieldType.size) {
                bitFieldType = typeIt->second;
                bitFieldOffset = 0;

                for (auto siblingIt = std::next(fieldIt); siblingIt != message.fields.end(); siblingIt++) {
                    const Attribute* siblingBits = siblingIt->attributes.Get("bits");
                    if (!siblingBits) {
                        break;
                    }

                    auto siblingTypeIt = primitiveTypeMap.types.find(siblingIt->type);
                    if (siblingTypeIt == primitiveTypeMap.types.end()) {
                        std::cerr << "Malformed command in line: " << message.line << ", type '" << siblingIt->type << "' does not support bit attribute" << std::endl;
                        return false;
                    }

                    if (siblingTypeIt->second.size > bitFieldType.size) {
                        bitFieldType = siblingTypeIt->second;
                    }
                }
            }
        } else {
            bitFieldType = {};
            bitFieldOffset = 0;
        }

        // Primitive?
        if (auto it = primitiveTypeMap.types.find(field.type); it != primitiveTypeMap.types.end()) {
            if (bits) {
                int32_t bitCount = std::atoi(bits->value.c_str());

                const uint32_t bitSize = static_cast<uint32_t>(bitFieldType.size) * 8;

                out.members << "\t\tpublic " << bitFieldType.csType << " " << field.name << " : " << bitCount << "\n";
                out.members << "\t\t{\n";
                out.members << "\t\t\tget => MemoryMarshal.Read<" << bitFieldType.csType << ">(_memory.Slice(" << cxxSizeType << ", " << (cxxSizeType + bitFieldType.size) << ").AsRefSpan());\n";
                out.members << "\t\t}\n\n";

                if (!bitFieldOffset || bitFieldOffset % bitSize == 0) {
                    cxxSizeType += bitFieldType.size;
                    byteSize << "\t\t\t\tsize += " << bitFieldType.size << ";\n";
                }

                const uint32_t bitElementBefore = static_cast<uint32_t>(bitFieldOffset / bitFieldType.size);
                const uint32_t bitElementAfter = static_cast<uint32_t>(bitFieldOffset / bitFieldType.size);

                if (bitElementAfter > bitElementBefore && (bitFieldOffset && bitFieldOffset % bitSize != 0)) {
                    std::cerr << "Malformed command in line: " << field.line << ", bit field size exceeded type size of " << bitFieldType.size << std::endl;
                    return false;
                }

                bitFieldOffset += bitCount;
            } else {
                byteSize << "\t\t\t\tsize += " << it->second.size << ";\n";

                out.members << "\t\tpublic " << it->second.csType << " " << field.name << "\n";
                out.members << "\t\t{\n";
                out.members << "\t\t\tget => MemoryMarshal.Read<" << it->second.csType << ">(_memory.Slice(" << cxxSizeType << ", " << (cxxSizeType + it->second.size) << ").AsRefSpan());\n";
                out.members << "\t\t}\n\n";

                cxxSizeType += it->second.size;
            }
        } else if (field.type ==  "array") {
            // Get the type
            const Attribute* elementTypeName = field.attributes.Get("element");
            if (!elementTypeName) {
                std::cerr << "Malformed command in line: " << field.line << ", element type not found" << std::endl;
                return false;
            }

            // Get the inbuilt type
            if (it = primitiveTypeMap.types.find(elementTypeName->value); it == primitiveTypeMap.types.end()) {
                std::cerr << "Malformed command in line: " << field.line << ", unknown type '" << elementTypeName->value << "'" << std::endl;
                return false;
            }

            // Add allocation parameter
            allocationParameters << "\t\t\tulong " << field.name << "Count;\n";

            // Add byte size
            byteSize << "\t\t\t\tulong += 16 + " << it->second.size << " * " << field.name << "Count" << ";\n";

            // Add patch
            patch << "\t\t\t\tmessage." << field.name << ".count = " << field.name << "Count;\n";
            patch << "\t\t\t\tmessage." << field.name << ".thisOffset = offset + (ulong)Marshal.SizeOf(typeof(" << message.name << "Message)) - " << cxxSizeType << ";\n";
            patch << "\t\t\t\toffset += " << field.name << "Count * " << it->second.size << "; \n\n";

            // Requires the dynamic schema
            anyDynamic = true;

            // Append field
            out.members << "\t\tpublic MessageArray<" << it->second.csType << "> " << field.name << "\n";
            out.members << "\t\t{\n";
            out.members << "\t\t\tget => new MessageArray<" << it->second.csType << ">{ Memory = _memory.Slice(" << cxxSizeType << ") };\n";
            out.members << "\t\t}\n\n";

            // Size type, not allocation type
            cxxSizeType += 16;
        } else if (field.type ==  "string") {
            // Add allocation parameter
            allocationParameters << "\t\t\tulong " << field.name << "Length;\n";

            // Add byte size
            byteSize << "\t\t\t\tsize += 16 + (ulong)Marshal.SizeOf(typeof(char)) * " << field.name << "Length" << ";\n";

            // Add patch
            patch << "\t\t\t\tmessage." << field.name << ".data.count = " << field.name << "Length;\n";
            patch << "\t\t\t\tmessage." << field.name << ".data.thisOffset = offset + (ulong)Marshal.SizeOf(typeof(" << message.name << "Message)) - " << cxxSizeType << ";\n";
            patch << "\t\t\t\toffset += " << field.name << "Length * (ulong)Marshal.SizeOf(typeof(char)); \n\n";

            // Requires the dynamic schema
            anyDynamic = true;

            // Append field
            out.members << "\t\tpublic MessageString " << field.name << "\n";
            out.members << "\t\t{\n";
            out.members << "\t\t\tget => new MessageString { Memory = _memory.Slice(" << cxxSizeType << ") };\n";
            out.members << "\t\t}\n\n";

            // Size type, not allocation type
            cxxSizeType += 16;
        } else if (field.type ==  "stream") {
            // Add allocation parameter
            allocationParameters << "\t\t\tulong " << field.name << "ByteSize;\n";

            // Add byte size
            byteSize << "\t\t\t\tsize += 32 + (ulong)Marshal.SizeOf(typeof(char)) * " << field.name << "ByteSize" << ";\n";

            // Add patch
            patch << "\t\t\t\tmessage." << field.name << ".data.count = " << field.name << "ByteSize;\n";
            patch << "\t\t\t\tmessage."
                  << field.name << ".data.thisOffset = offset"
                  << " + (ulong)Marshal.SizeOf(typeof(" << message.name << "Message))"
                  << " - " << cxxSizeType
                  << " - 16;\n";
            patch << "\t\t\t\toffset += " << field.name << "ByteSize * (ulong)Marshal.SizeOf(typeof(char)); \n\n";

            // Requires the dynamic schema
            anyDynamic = true;

            // Append field
            out.members << "\t\tpublic MessageSubStream " << field.name << "\n";
            out.members << "\t\t{\n";
            out.members << "\t\t\tget => new MessageSubStream { Memory = _memory.Slice(" << cxxSizeType << ") };\n";
            out.members << "\t\t}\n\n";

            // Size type, not allocation type
            cxxSizeType += 32;
        } else if (auto it = declaredTypes.find(field.type); it != declaredTypes.end()) {
            // Increase size
            byteSize << "\t\t\t\tsize += " << it->second.size << ";\n";

            // Add field
            out.members << "\t\tpublic " << it->first << " " << field.name << "\n";
            out.members << "\t\t{\n";
            out.members << "\t\t\tget => MemoryMarshal.Read<" << it->first << ">(_memory.Slice(" << cxxSizeType << ", " << (cxxSizeType + it->second.size) << ").AsRefSpan()); }\n";
            out.members << "\t\t}\n\n";

            cxxSizeType += it->second.size;
        } else {
            std::cerr << "Malformed command in line: " << message.line << ", unknown type '" << field.type << "'" << std::endl;
            return false;
        }
    }

    // Append to type map
    TypeMeta meta;
    meta.size = cxxSizeType;
    declaredTypes[message.name] = meta;

    // Set schema
    if (anyDynamic) {
        out.schemaType << "IDynamicMessageSchema";
    } else {
        out.schemaType << "IStaticMessageSchema";
    }

    // Set size
    out.size = cxxSizeType;

    // Begin allocation info
    out.types << "\n";
    out.types << "\t\tstruct AllocationInfo {\n";

    // Byte size information
    out.types << "\t\t\tulong ByteSize() {\n";
    out.types << "\t\t\t\tulong size = 0;\n";
    out.types << byteSize.str();
    out.types << "\t\t\t\treturn size;\n";
    out.types << "\t\t\t}\n";

    // Allocation patching
    out.types << "\n";
    out.types << "#if false // Read only for now\n";
    out.types << "\t\t\tvoid Patch(ref " << message.name << "Message message) {\n";
    out.types << "\t\t\t\tulong offset = 0;\n";
    out.types << patch.str();
    out.types << "\t\t\t}\n";
    out.types << "#endif";

    // Allocation parameters
    out.types << "\n\n";
    out.types << allocationParameters.str();

    // End allocation info
    out.types << "\t\t};\n";

    // OK
    return true;
}
