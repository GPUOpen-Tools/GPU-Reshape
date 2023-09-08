// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

#include "GenTypes.h"

// Std
#include <iostream>
#include <array>
#include <map>

// Common
#include <Common/Assert.h>
#include <Common/String.h>

/// Extension metadata
struct ExtensionMetadata {
    std::string name;
    std::string structureType;
    tinyxml2::XMLElement *typeNode;
};

/// Generator metadata
struct ObjectTreeMetadata {
    std::set<std::string> typeNames;
    std::map<std::string, tinyxml2::XMLElement *> lookup;
    std::map<std::string, ExtensionMetadata> extensions;
};

/// State of a given deep copy
struct DeepCopyState {
    uint32_t counter{0};

    std::stringstream byteSize;
    std::stringstream deepCopy;
};

/// Pad a string
static std::string Pad(uint32_t n) {
    return std::string(n, '\t');
}

/// Naive tokenizer
/// \param token the current token to be iterated
/// \param separators the accepted separators
/// \return length of the token
static uint32_t StrTokKeepSeparators(const char *&token, const char *separators) {
    const uint32_t separatorLength = static_cast<uint32_t>(std::strlen(separators));

    // Start offset
    const char *start = token;

    // Consume until separator, or zero terminator
    for (; *token; ++token) {
        for (uint32_t i = 0; i < separatorLength; i++) {
            if (*token == separators[i]) {
                // Consume separator
                ++token;

                // Return size
                return static_cast<uint32_t>(token - start);
            }
        }
    }

    // Return size
    return static_cast<uint32_t>(token - start);
}

/// Parse a specification array length
/// \param out the stream
/// \param accessorPrefix the prefix for all local accessors
/// \param str length string
/// \return success state
[[nodiscard]]
static bool ParseArrayLength(std::stringstream &out, const std::string &accessorPrefix, const char *str) {
    // Standard separators
    constexpr char separators[] = " ,\t\n()";

    // Get the first token
    while (uint32_t len = StrTokKeepSeparators(str, separators)) {
        const char *segment = str - len;

        // Identifier? Upper case denotes macro.
        if (std::isalpha(*segment) && !std::isupper(*segment)) {
            out << accessorPrefix;
        }

        // Write name
        out.write(segment, len);
    }

    // OK
    return true;
}

/// Assign an indirection to the creation state, and create a local mutable representation
/// \param state the current state
/// \param accessorPrefix destination accessor prefix
/// \param memberType type of the member
/// \param memberName name of the member
/// \param indent current indentation level
/// \return the mutable variable name
std::string AssignPtrAndGetMutable(DeepCopyState &state, const std::string &accessorPrefix, tinyxml2::XMLElement *memberType, tinyxml2::XMLElement *memberName, uint32_t indent) {
    std::string mutableName = "mutable" + std::to_string(state.counter++);

    // Create mutable
    state.deepCopy << Pad(indent) << memberType->GetText() << "* " << mutableName << " = ";
    state.deepCopy << "reinterpret_cast<" << memberType->GetText() << "* " << ">";
    state.deepCopy << "(&blob[blobOffset]);\n";

    // Assign it
    state.deepCopy << Pad(indent) << accessorPrefix << memberName->GetText() << " = " << mutableName << ";\n";

    return mutableName;
}

/// Perform a deep copy of an object tree
/// \param md the current metadata state
/// \param state the current deep copy state
/// \param type the type node
/// \param sourceAccessorPrefix the source creation info accessor
/// \param destAccessorPrefix the destination creation info accessor
/// \param indent the current indentation level
/// \return success state
[[nodiscard]]
static bool DeepCopyObjectTree(ObjectTreeMetadata &md, DeepCopyState &state, tinyxml2::XMLElement *type, const std::string &sourceAccessorPrefix = "source.", const std::string &destAccessorPrefix = "createInfo.", uint32_t indent = 1, bool alwaysEmitSize = false) {
    const char *name = type->Attribute("name", nullptr);
    if (!name) {
        std::cerr << "Malformed type in line: " << type->GetLineNum() << ", name not found" << std::endl;
        return false;
    }

    // Add to type set
    md.typeNames.insert(name);
    
    // Go through all members
    for (tinyxml2::XMLElement *memberNode = type->FirstChildElement("member"); memberNode; memberNode = memberNode->NextSiblingElement("member")) {
        // Get the type
        tinyxml2::XMLElement *memberType = memberNode->FirstChildElement("type");
        if (!memberType) {
            std::cerr << "Malformed type in line: " << memberNode->GetLineNum() << ", type not found" << std::endl;
            return false;
        }

        // Get the name
        tinyxml2::XMLElement *memberName = memberNode->FirstChildElement("name");
        if (!memberName) {
            std::cerr << "Malformed type in line: " << memberNode->GetLineNum() << ", name not found" << std::endl;
            return false;
        }

        // Ignore next pointers
        if (!std::strcmp(memberName->GetText(), "pNext")) {
            // Add check
            state.byteSize << Pad(indent) << "if (" << sourceAccessorPrefix << memberName->GetText() << ") {\n";
            state.byteSize << Pad(indent + 1) << "blobSize += DeepCopyExtensionByteSize(" << sourceAccessorPrefix << memberName->GetText() << ");\n";
            state.byteSize << Pad(indent) << "}\n";

            state.deepCopy << "\n" << Pad(indent) << "// " << sourceAccessorPrefix << memberName->GetText() << "\n";
            state.deepCopy << Pad(indent) << "if (" << sourceAccessorPrefix << memberName->GetText() << ") {\n";
            state.deepCopy << Pad(indent + 1) << destAccessorPrefix << memberName->GetText() << " = DeepCopyExtension(" << sourceAccessorPrefix << memberName->GetText() << ", blob, blobOffset);\n";
            state.deepCopy << Pad(indent) << "} else {\n";
            state.deepCopy << Pad(indent + 1) << destAccessorPrefix << memberName->GetText() << " = nullptr;\n";
            state.deepCopy << Pad(indent) << "}\n";
            continue;
        }

        // Indirection?
        bool isIndirection{false};
        bool isArray{false};

        // Array size
        std::string arrayCount = "0";

        // Postfixes?
        for (auto postfix = memberType->NextSibling(); postfix != memberName; postfix = postfix->NextSibling()) {
            if (tinyxml2::XMLText *text = postfix->ToText()) {
                std::string trimmed = std::trim_copy(text->Value());

                if (trimmed == "*") {
                    isIndirection = true;
                }
            }
        }

        // Array?
        for (auto array = memberName->NextSibling(); array; array = array->NextSibling()) {
            if (tinyxml2::XMLText *text = array->ToText()) {
                std::string trimmed = std::trim_copy(text->Value());

                if (!trimmed.empty() && trimmed[0] == '[') {
                    isArray = true;

                    uint64_t end = trimmed.find(']');
                    if (end != std::string::npos) {
                        arrayCount = trimmed.substr(1, end - 1);
                    } else if (auto next = array->NextSibling()) {
                        const char* value = next->Value();

                        if (value && !std::strcmp(value, "enum")) {
                            arrayCount = next->ToElement()->GetText();
                        } else {
                            std::cerr << "Malformed type in line: " << memberNode->GetLineNum() << ", unexpected contained array type '" << value << "'" << std::endl;
                            return false;
                        }
                    } else {
                        std::cerr << "Malformed type in line: " << memberNode->GetLineNum() << ", array length end not found" << std::endl;
                        return false;
                    }
                }
            }
        }

        // Comments
        state.deepCopy << "\n" << Pad(indent) << "// " << sourceAccessorPrefix << memberName->GetText() << "\n";

        // Indirection?
        if (isIndirection) {
            // Additional attributes
            const char *optional = memberNode->Attribute("optional");
            const char *noAutoValidity = memberNode->Attribute("noautovalidity");

            // Optional?
            const bool isOptional = optional && !std::strcmp(optional, "true");
            const bool isNoAutoValidity = noAutoValidity && !std::strcmp(noAutoValidity, "true");

            // May be null?
            const bool canBeNull = isOptional || isNoAutoValidity;

            // Wrap in check
            if (canBeNull) {
                const char *reason = isNoAutoValidity ? "no-auto-validity" : "optional";

                state.byteSize << Pad(indent) << "if (" << sourceAccessorPrefix + memberName->GetText() << ") /* " << reason << " */ {\n";
                state.deepCopy << Pad(indent) << "if (" << sourceAccessorPrefix + memberName->GetText() << ") /* " << reason << " */ {\n";
                indent++;
            }

            // Get the length, try alt-len first
            const char *length = memberNode->Attribute("altlen", nullptr);

            // Try length if not possible
            if (!length) {
                length = memberNode->Attribute("len", nullptr);
            }

            // Array?
            if (length) {
                // C String?
                // Note: null-terminated could imply other things, but the standard is only using it for strings at the moment, something to watch out for
                if (std::strstr(length, "null-terminated")) {
                    // Size variable
                    std::string sizeVar = "size_" + std::to_string(state.counter++);
                    state.byteSize << Pad(indent) << "uint64_t " << sizeVar << " = std::strlen(" << sourceAccessorPrefix << memberName->GetText() << ") + 1;\n";

                    // Repeat for deep copy if scope requires it
                    if (indent > 1 || alwaysEmitSize) {
                        state.deepCopy << Pad(indent) << "uint64_t " << sizeVar << " = std::strlen(" << sourceAccessorPrefix << memberName->GetText() << ") + 1;\n";
                    }

                    // Byte size
                    state.byteSize << Pad(indent) << "blobSize += sizeof(char) * " << sizeVar << ";\n";

                    // Assign the indirection
                    std::string mutableName = AssignPtrAndGetMutable(state, destAccessorPrefix, memberType, memberName, indent);

                    // Copy
                    state.deepCopy << Pad(indent) << "std::memcpy(" << mutableName << ", " << sourceAccessorPrefix << memberName->GetText();
                    state.deepCopy << ", sizeof(char) * " << sizeVar << ");\n";

                    // Offset
                    state.deepCopy << Pad(indent) << "blobOffset += sizeof(char) * " << sizeVar << ";\n";
                } else {
                    // Standard array

                    // Size variable
                    std::string sizeVar = "size_" + std::to_string(state.counter++);

                    // Try to parse the length
                    std::stringstream lengthStream;
                    if (!ParseArrayLength(lengthStream, sourceAccessorPrefix, length)) {
                        return false;
                    }

                    state.byteSize << Pad(indent) << "uint64_t " << sizeVar << " = " << lengthStream.str() << ";\n";

                    // Repeat for deep copy if scope requires it
                    if (indent > 1 || alwaysEmitSize) {
                        state.deepCopy << Pad(indent) << "uint64_t " << sizeVar << " = " << lengthStream.str() << ";\n";
                    }

                    // Standard pointer byte size
                    std::string sizeType;
                    if (!std::strcmp(memberType->GetText(), "void")) {
                        sizeType = "uint8_t";
                    } else {
                        sizeType = "*" + sourceAccessorPrefix + memberName->GetText();
                    }

                    state.byteSize << Pad(indent) << "blobSize += sizeof(" << sizeType << ") * " << sizeVar << ";\n";

                    // Find the element type
                    auto elementType = md.lookup.find(memberType->GetText());

                    // Assign the indirection
                    std::string mutableName = AssignPtrAndGetMutable(state, destAccessorPrefix, memberType, memberName, indent);

                    // If end, this is a POD type
                    if (elementType == md.lookup.end()) {
                        state.deepCopy << Pad(indent) << "std::memcpy(" << mutableName << ", " << "" << sourceAccessorPrefix << memberName->GetText();
                        state.deepCopy << ", sizeof(" << sizeType << ") * " << sizeVar << ");\n";

                        // Offset
                        state.deepCopy << Pad(indent) << "blobOffset += sizeof(" << sizeType << ") * " << sizeVar << ";\n";
                    } else {
                        // Offset
                        state.deepCopy << Pad(indent) << "blobOffset += sizeof(" << sizeType << ") * " << sizeVar << ";\n";

                        // Counter var
                        std::string counterVar = "i" + std::to_string(state.counter++);

                        // Begin the loops
                        state.byteSize << Pad(indent) << "for (size_t " << counterVar << " = 0; " << counterVar << " < " << sizeVar << "; " << counterVar << "++) {\n";
                        state.deepCopy << Pad(indent) << "for (size_t " << counterVar << " = 0; " << counterVar << " < " << sizeVar << "; " << counterVar << "++) {\n";

                        // Copy the tree
                        if (!DeepCopyObjectTree(md, state, elementType->second, sourceAccessorPrefix + memberName->GetText() + "[" + counterVar + "].", mutableName + "[" + counterVar + "].", indent + 1, alwaysEmitSize)) {
                            return false;
                        }

                        // End the loops
                        state.byteSize << Pad(indent) << "}\n";
                        state.deepCopy << Pad(indent) << "}\n";
                    }
                }
            } else {
                // Standard pointer byte size
                std::string sizeType;
                if (!std::strcmp(memberType->GetText(), "void")) {
                    sizeType = "uint8_t";
                } else {
                    sizeType = "*" + sourceAccessorPrefix + memberName->GetText();
                }

                // Standard pointer byte size
                state.byteSize << Pad(indent) << "blobSize += sizeof(" << sizeType << ");\n";

                // Find the element type
                auto elementType = md.lookup.find(memberType->GetText());

                // Assign the indirection
                std::string mutableName = AssignPtrAndGetMutable(state, destAccessorPrefix, memberType, memberName, indent);

                // Offset
                state.deepCopy << Pad(indent) << "blobOffset += sizeof(" << sizeType << ");\n";

                // If end, this is a POD type
                if (elementType == md.lookup.end()) {
                    state.deepCopy << Pad(indent) << "std::memcpy(" << mutableName << ", " << sourceAccessorPrefix << memberName->GetText();
                    state.deepCopy << ", sizeof(" << sizeType << "));\n";
                } else {
                    // Copy the tree
                    if (!DeepCopyObjectTree(md, state, elementType->second, sourceAccessorPrefix + memberName->GetText() + "->", mutableName + "->", indent, alwaysEmitSize)) {
                        return false;
                    }
                }
            }

            // Wrap in check
            if (canBeNull) {
                indent--;

                state.byteSize << Pad(indent) << "}\n";

                // If not specified, set mutable state to nullptr
                state.deepCopy << Pad(indent) << "} else {\n";
                state.deepCopy << Pad(indent + 1) << destAccessorPrefix << memberName->GetText() << " = nullptr;\n";
                state.deepCopy << Pad(indent) << "}\n";
            }
        } else if (isArray) {
            // Find the element type
            auto elementType = md.lookup.find(memberType->GetText());

            // If end, this is a POD type
            if (elementType == md.lookup.end()) {
                // POD array copy
                state.deepCopy << Pad(indent) << "std::memcpy(" << destAccessorPrefix << memberName->GetText() << ", " << sourceAccessorPrefix << memberName->GetText();
                state.deepCopy << ", sizeof(" << sourceAccessorPrefix + memberName->GetText() << "));\n";
            } else {
                // Counter var
                std::string counterVar = "i" + std::to_string(state.counter++);
                state.deepCopy << Pad(indent) << "for (size_t " << counterVar << " = 0; " << counterVar << " < " << arrayCount << "; " << counterVar << "++) {\n";

                // Copy the tree
                if (!DeepCopyObjectTree(
                    md, state, elementType->second,
                    sourceAccessorPrefix + memberName->GetText() + "[" + counterVar + "]",
                    destAccessorPrefix + memberName->GetText() + "[" + counterVar + "]",
                    indent, alwaysEmitSize
                )) {
                    return false;
                }

                state.deepCopy << Pad(indent) << "}\n";
            }

        } else {
            // Find the element type
            auto elementType = md.lookup.find(memberType->GetText());

            // If end, this is a POD type
            if (elementType == md.lookup.end()) {
                state.deepCopy << Pad(indent) << destAccessorPrefix << memberName->GetText() << " = " << sourceAccessorPrefix << memberName->GetText() << ";\n";
            } else {
                // Copy the tree
                if (!DeepCopyObjectTree(md, state, elementType->second, sourceAccessorPrefix + memberName->GetText() + ".", destAccessorPrefix + memberName->GetText() + ".", indent, alwaysEmitSize)) {
                    return false;
                }
            }
        }
    }

    // OK
    return true;
}

static bool CollectDeepCopyExtensionStructures(ObjectTreeMetadata& md, tinyxml2::XMLElement *types, const std::string_view& parent) {
    // Create extension lookup
    for (tinyxml2::XMLElement *typeNode = types->FirstChildElement("type"); typeNode; typeNode = typeNode->NextSiblingElement("type")) {
        // Try to get the category, if not, not interested
        const char *category = typeNode->Attribute("category", nullptr);
        if (!category) {
            continue;
        }

        // Skip non-compound types
        if (std::strcmp(category, "struct") != 0) {
            continue;
        }

        // Attempt to get the name
        const char *name = typeNode->Attribute("name", nullptr);
        if (!name) {
            std::cerr << "Malformed type in line: " << typeNode->GetLineNum() << ", name not found" << std::endl;
            return false;
        }

        // Try to get the extensions, if not, not interested
        const char *extensions = typeNode->Attribute("structextends", nullptr);
        if (!extensions) {
            continue;
        }

        // Does this type extend the parent?
        bool anyUse{false};

        // Parse all structs
        std::stringstream ss(extensions);
        while (ss.good()) {
            std::string _struct;
            getline(ss, _struct, ',');

            // Potential extension type?
            if (_struct == parent) {
                anyUse = true;
                break;
            }
        }

        // Next?
        if (!anyUse) {
            continue;
        }

        // Add to lookup
        ExtensionMetadata ext;
        ext.name = name;
        ext.typeNode = typeNode;

        // Go through all members
        for (tinyxml2::XMLElement *memberNode = typeNode->FirstChildElement("member"); memberNode; memberNode = memberNode->NextSiblingElement("member")) {
            // Get the type
            tinyxml2::XMLElement *memberType = memberNode->FirstChildElement("type");
            if (!memberType) {
                std::cerr << "Malformed type in line: " << memberNode->GetLineNum() << ", type not found" << std::endl;
                return false;
            }

            // Found it?
            if (std::strcmp(memberType->GetText(), "VkStructureType")) {
                continue;
            }

            // Set structure type
            if (const char* attr = memberNode->Attribute("values", nullptr)) {
                ext.structureType = attr;
                break;
            }
        }

        // Must have type
        if (ext.structureType.empty()) {
            std::cerr << "Malformed type in line: " << typeNode->GetLineNum() << ", structure type not found" << std::endl;
            return false;
        }

        // Add
        md.extensions[name] = ext;

        // Collect nested copies
        CollectDeepCopyExtensionStructures(md, types, name);
    }

    // OK
    return true;
}

static bool CreateExtensions(ObjectTreeMetadata& md, tinyxml2::XMLElement *types, TemplateEngine& templateEngine) {
    std::stringstream cases;
    std::stringstream casesByteSize;
    std::stringstream extensions;

    // Generate cases
    for (auto&& kv : md.extensions) {
        casesByteSize << "\t\tcase " << kv.second.structureType << ":\n";
        casesByteSize << "\t\t\treturn DeepCopyExtensionByteSize" << kv.first << "(extension);\n";

        cases << "\t\tcase " << kv.second.structureType << ":\n";
        cases << "\t\t\treturn DeepCopyExtension" << kv.first << "(extension, blob, blobOffset);\n";
    }

    // Generate copies
    for (auto&& kv : md.extensions) {
        ExtensionMetadata& ext = kv.second;

        // Attempt to generate a deep copy
        DeepCopyState state;
        if (!DeepCopyObjectTree(md, state, ext.typeNode, "source.", "_mutable->", 1, true)) {
            return false;
        }

        extensions << "uint64_t DeepCopyExtensionByteSize" << kv.first << "(const void* extension) {\n";
        extensions << "\tconst " << kv.first << "& source = *static_cast<const " << kv.first << "*>(extension);\n\n";
        extensions << "\tuint64_t blobSize = sizeof(" << kv.first << ");\n\n";
        extensions << state.byteSize.str();
        extensions << "\n\treturn blobSize;\n";
        extensions << "}\n\n";

        extensions << "void* DeepCopyExtension" << kv.first << "(const void* extension, uint8_t* blob, uint64_t& blobOffset) {\n";
        extensions << "\tconst " << kv.first << "& source = *static_cast<const " << kv.first << "*>(extension);\n\n";
        extensions << "\tauto* _mutable = reinterpret_cast<" << kv.first << "*>(&blob[blobOffset]);\n";
        extensions << "\tstd::memcpy(_mutable, extension, sizeof(" << kv.first << "));\n";
        extensions << "\tblobOffset += sizeof(" << kv.first << ");\n\n";
        extensions << state.deepCopy.str();
        extensions << "\n\treturn _mutable;\n";
        extensions << "}\n\n";
    }

    // Replace
    templateEngine.Substitute("$EXTENSION_CASES_BYTE_SIZE", casesByteSize.str().c_str());
    templateEngine.Substitute("$EXTENSION_CASES", cases.str().c_str());
    templateEngine.Substitute("$EXTENSIONS", extensions.str().c_str());

    // OK
    return true;
}

bool Generators::DeepCopy(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    // Get the types
    tinyxml2::XMLElement *types = info.registry->FirstChildElement("types");
    if (!types) {
        std::cerr << "Failed to find commands in registry" << std::endl;
        return false;
    }

    // Metadata
    ObjectTreeMetadata md;

    // Populate lookup
    for (tinyxml2::XMLElement *typeNode = types->FirstChildElement("type"); typeNode; typeNode = typeNode->NextSiblingElement("type")) {
        // Try to get the category, if not, not interested
        const char *category = typeNode->Attribute("category", nullptr);
        if (!category) {
            continue;
        }

        // Skip non-compound types
        if (std::strcmp(category, "struct") != 0) {
            continue;
        }

        // Attempt to get the name
        const char *name = typeNode->Attribute("name", nullptr);
        if (!name) {
            std::cerr << "Malformed type in line: " << typeNode->GetLineNum() << ", name not found" << std::endl;
            return false;
        }

        // Add lookup
        md.lookup[name] = typeNode;
    }

    // Final stream
    std::stringstream deepCopy;

    // Create deep copies
    for (tinyxml2::XMLElement *typeNode = types->FirstChildElement("type"); typeNode; typeNode = typeNode->NextSiblingElement("type")) {
        // Try to get the category, if not, not interested
        const char *category = typeNode->Attribute("category", nullptr);
        if (!category) {
            continue;
        }

        // Skip non-compound types
        if (std::strcmp(category, "struct") != 0) {
            continue;
        }

        // Attempt to get the name
        const char *name = typeNode->Attribute("name", nullptr);
        if (!name) {
            std::cerr << "Malformed type in line: " << typeNode->GetLineNum() << ", name not found" << std::endl;
            return false;
        }

        // Candidate?
        if (!info.objects.count(name)) {
            continue;
        }

        // Attempt to generate a deep copy
        DeepCopyState state;
        if (!DeepCopyObjectTree(md, state, typeNode)) {
            return false;
        }

        // Begin deep copy constructor
        deepCopy << "void " << name << "DeepCopy::DeepCopy(const Allocators& _allocators, const " << name << "& source) {\n";
        deepCopy << "\tallocators = _allocators;\n";

        // Byte size
        deepCopy << "\t// Byte size\n";
        deepCopy << "\tuint64_t blobSize = 0;\n";
        deepCopy << state.byteSize.str();

        deepCopy << "\n\t// Create the blob allocation\n";
        deepCopy << "\tif (length < blobSize) {\n";
        deepCopy << "\t\tdestroy(blob, allocators);\n\n";
        deepCopy << "\t\tblob = new (allocators) uint8_t[blobSize];\n";
        deepCopy << "\t\tlength = blobSize;\n";
        deepCopy << "\t}\n";

        // Deep copy
        deepCopy << "\n\t// Create the deep copies\n";
        deepCopy << "\tuint64_t blobOffset = 0;\n";
        deepCopy << state.deepCopy.str();

        // Safety check
        deepCopy << "\n\tASSERT(blobSize == blobOffset, \"Size / Offset mismatch, deep copy failed\");\n";

        // End deep copy constructor
        deepCopy << "}\n\n";

        // Begin destructor
        deepCopy << name << "DeepCopy::~" << name << "DeepCopy() {\n";

        // Release the blob
        deepCopy << "\tif (blob) {\n";
        deepCopy << "\t\tdestroy(blob, allocators);\n";
        deepCopy << "\t}\n";

        // End destructor
        deepCopy << "}\n\n";
    }

    // Collect all extensions
    for (const std::string& object : md.typeNames) {
        if (!CollectDeepCopyExtensionStructures(md, types, object)) {
            return false;
        }
    }

    // Create extensions
    if (!CreateExtensions(md, types, templateEngine)) {
        return false;
    }

    // Instantiate template
    if (!templateEngine.Substitute("$DEEPCOPY", deepCopy.str().c_str())) {
        std::cerr << "Bad template, failed to substitute $DEEPCOPY" << std::endl;
        return false;
    }

    // OK
    return true;
}
