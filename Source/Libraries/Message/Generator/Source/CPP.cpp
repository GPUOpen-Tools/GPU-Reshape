#include "GenTypes.h"

// Common
#include <Common/IDHash.h>

// Std
#include <iostream>
#include <array>
#include <map>

bool Generators::CPP(const GeneratorInfo& info, TemplateEngine& templateEngine) {
    // Template streams
    std::stringstream messages;

    // Process all commands
    for (tinyxml2::XMLNode *commandNode = info.schema->FirstChild(); commandNode; commandNode = commandNode->NextSibling()) {
        tinyxml2::XMLElement *command = commandNode->ToElement();

        // Get the name
        const char* name = command->Attribute("name", nullptr);
        if (!name) {
            std::cerr << "Malformed command in line: " << command->GetLineNum() << ", name not found" << std::endl;
            return false;
        }

        // Members of the message
        std::stringstream fields;
        std::stringstream patch;
        std::stringstream byteSize;
        std::stringstream allocationParameters;

        // Any dynamic parameters?
        bool anyDynamic = false;

        // Total size
        uint64_t cxxSizeType = 0;

        // Iterate all parameters
        for (tinyxml2::XMLNode *childNode = commandNode->FirstChild(); childNode; childNode = childNode->NextSibling()) {
            tinyxml2::XMLElement* element = childNode->ToElement();
            if (!element) {
                std::cerr << "Malformed command in line: " << childNode->GetLineNum() << std::endl;
                return false;
            }

            // Field type?
            if (!std::strcmp(element->Name(), "field")) {
                // Get the name
                const char* fieldName = element->Attribute("name", nullptr);
                if (!fieldName) {
                    std::cerr << "Malformed command in line: " << element->GetLineNum() << ", name not found" << std::endl;
                    return false;
                }

                // Get the type
                const char* typeName = element->Attribute("type", nullptr);
                if (!typeName) {
                    std::cerr << "Malformed command in line: " << element->GetLineNum() << ", type not found" << std::endl;
                    return false;
                }

                // Get the default value
                const char* defaultValue = element->Attribute("value", nullptr);

                // Type information
                struct TypeInfo
                {
                    const char* cxxType;
                    uint64_t    size;
                };

                // Standard types
                static const std::map<std::string, TypeInfo> inbuiltTypes = {
                    { "uint64", { "uint64_t", 8 } },
                    { "uint32", { "uint32_t", 4 } },
                    { "uint16", { "uint16_t", 2 } },
                    { "uint8",  { "uint8_t",  1 } },
                    { "int64",  { "int64_t",  8 } },
                    { "int32",  { "int32_t",  4 } },
                    { "int16",  { "int16_t",  2 } },
                    { "int8",   { "int8_t",   1 } },
                    { "float",  { "float",    4 } },
                    { "double", { "double",   8 } },
                };

                // Primitive?
                if (auto it = inbuiltTypes.find(typeName); it != inbuiltTypes.end()) {
                    cxxSizeType += it->second.size;

                    byteSize << "\t\t\tsize += " << it->second.size << ";\n";

                    fields << "\t" << it->second.cxxType << " " << fieldName;

                    if (defaultValue) {
                        fields << " = " << defaultValue;
                    }

                    fields << ";\n";
                } else if (!std::strcmp(typeName, "array")) {
                    // Get the type
                    const char* elementTypeName = element->Attribute("element", nullptr);
                    if (!elementTypeName) {
                        std::cerr << "Malformed command in line: " << element->GetLineNum() << ", element type not found" << std::endl;
                        return false;
                    }

                    // Get the inbuilt type
                    if (it = inbuiltTypes.find(elementTypeName); it == inbuiltTypes.end()) {
                        std::cerr << "Malformed command in line: " << element->GetLineNum() << ", unknown type '" << elementTypeName << "'" << std::endl;
                        return false;
                    }

                    // Add allocation parameter
                    allocationParameters << "\t\tsize_t " << fieldName << "Count;\n";

                    // Add byte size
                    byteSize << "\t\t\tsize += 16 + " << it->second.size << " * " << fieldName << "Count" << ";\n";

                    // Add patch
                    patch << "\t\t\tmessage->" << fieldName << ".count = " << fieldName << "Count;\n";
                    patch << "\t\t\tmessage->" << fieldName << ".thisOffset = offset + sizeof(" << name << "Message) - offsetof(" << name << "Message, " << fieldName << ");\n";
                    patch << "\t\t\toffset += " << fieldName << "Count * " << it->second.size << "; \n\n";

                    // Requires the dynamic schema
                    anyDynamic = true;

                    // Append field
                    fields << "\tMessageArray<" << it->second.cxxType << "> " << fieldName << ";\n";

                    // Size type, not allocation type
                    cxxSizeType += 16;
                } else {
                    std::cerr << "Malformed command in line: " << command->GetLineNum() << ", unknown type '" << typeName << "'" << std::endl;
                    return false;
                }
            } else {
                std::cerr << "Malformed command in line: " << element->GetLineNum() << ", unknown xml type '" << element->Name() << "'" << std::endl;
                return false;
            }
        }

        // Message structure
        messages << "struct ALIGN_TO(4) " << name << "Message {\n";

        // Set schema
        if (anyDynamic) {
            messages << "\tusing Schema = DynamicMessageSchema;\n";
        } else {
            messages << "\tusing Schema = StaticMessageSchema;\n";
        }

        // Append id
        messages << "\n";
        messages << "\tstatic constexpr MessageID kID = " << IDHash(name) << "u;\n";

        // Begin allocation info
        messages << "\n";
        messages << "\tstruct AllocationInfo {\n";

        // Byte size information
        messages << "\t\t[[nodiscard]]\n";
        messages << "\t\tuint64_t ByteSize() const {\n";
        messages << "\t\t\tuint64_t size = 0;\n";
        messages << byteSize.str();
        messages << "\t\t\treturn size;\n";
        messages << "\t\t}\n";

        // Allocation patching
        messages << "\n";
        messages << "\t\tvoid Patch(" << name << "Message* message) const {\n";
        messages << "\t\t\tuint64_t offset = 0;\n";
        messages << patch.str();
        messages << "\t\t}";

        // Allocation parameters
        messages << "\n\n";
        messages << allocationParameters.str();

        // End allocation info
        messages << "\t};\n";

        // Local fields
        messages << "\n";
        messages << fields.str();
        messages << "};\n";

        // Size check
        messages << "static_assert(sizeof(" << name << "Message) == " << cxxSizeType << ", \"Unexpected compiler packing\");\n";

        // Space!
        messages << "\n";
    }

    // Instantiate hooks
    if (!templateEngine.Substitute("$MESSAGES", messages.str().c_str())) {
        std::cerr << "Bad template, failed to substitute $MESSAGES" << std::endl;
        return false;
    }

    // OK
    return true;
}
