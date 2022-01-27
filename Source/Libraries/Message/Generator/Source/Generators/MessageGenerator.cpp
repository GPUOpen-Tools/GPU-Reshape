#include <Generators/MessageGenerator.h>

#include <sstream>
#include <iostream>

bool MessageGenerator::Generate(Schema &schema, Language language, const SchemaStream &out) {
    return true;
}

bool MessageGenerator::Generate(const Message &message, Language language, const MessageStream &out) {
    switch (language) {
        case Language::CPP:
            return GenerateCPP(message, out);
        case Language::CS:
            return GenerateCS(message, out);
    }
}

bool MessageGenerator::GenerateCPP(const Message &message, const MessageStream &out) {
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

                const uint32_t bitSize = bitFieldType.size * 8;

                if (!bitFieldOffset || bitFieldOffset % bitSize == 0) {
                    cxxSizeType += bitFieldType.size;
                    byteSize << "\t\t\tsize += " << bitFieldType.size << ";\n";
                }

                out.members << "\t" << bitFieldType.cxxType << " " << field.name << " : " << bitCount << ";\n";

                const uint32_t bitElementBefore = bitFieldOffset / bitFieldType.size;
                const uint32_t bitElementAfter = bitFieldOffset / bitFieldType.size;

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
            patch << "\t\t\tmessage->" << field.name << ".data.thisOffset = offset + sizeof(" << message.name << "Message) - offsetof(" << message.name << "Message, " << field.name << ");\n";
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
    CxxTypeMeta meta;
    meta.size = cxxSizeType;
    declaredTypes[message.name] = meta;

    // Set schema
    if (anyDynamic) {
        out.types << "\tusing Schema = DynamicMessageSchema;\n";
    } else {
        out.types << "\tusing Schema = StaticMessageSchema;\n";
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

bool MessageGenerator::GenerateCS(const Message &message, const MessageStream &out) {
    return true;
}
