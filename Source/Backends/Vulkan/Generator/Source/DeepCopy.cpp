#include "GenTypes.h"

// Std
#include <iostream>
#include <array>
#include <map>

// Common
#include <Common/Assert.h>
#include <Common/Trim.h>

/// Generator metadata
struct ObjectTreeMetadata {
    std::map<std::string, tinyxml2::XMLElement *> lookup;
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
static uint32_t StrTokKeepSeparators(const char*& token, const char* separators) {
    const uint32_t separatorLength = std::strlen(separators);

    // Start offset
    const char* start = token;

    // Consume until separator, or zero terminator
    for (; *token; ++token) {
        for (uint32_t i = 0; i < separatorLength; i++) {
            if (*token == separators[i]) {
                // Consume separator
                ++token;

                // Return size
                return token - start;
            }
        }
    }

    // Return size
    return token - start;
}

/// Parse a specification array length
/// \param out the stream
/// \param accessorPrefix the prefix for all local accessors
/// \param str length string
/// \return success state
[[nodiscard]]
static bool ParseArrayLength(std::stringstream& out, const std::string& accessorPrefix, const char* str) {
    // Standard separators
    constexpr char separators[] = " ,\t\n()";

    // Get the first token
    while (uint32_t len = StrTokKeepSeparators(str, separators)) {
        const char* segment = str - len;

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
std::string AssignPtrAndGetMutable(DeepCopyState &state, const std::string& accessorPrefix, tinyxml2::XMLElement* memberType, tinyxml2::XMLElement* memberName, uint32_t indent) {
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
static bool DeepCopyObjectTree(ObjectTreeMetadata &md, DeepCopyState &state, tinyxml2::XMLElement *type, const std::string &sourceAccessorPrefix = "source.", const std::string &destAccessorPrefix = "createInfo.", uint32_t indent = 1) {
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
            state.byteSize << Pad(indent) << "ASSERT(!" << sourceAccessorPrefix << memberName->GetText() << ", \"Extension pointers not yet supported for deep copies\");\n";
            continue;
        }

        // Indirection?
        bool isIndirection{false};
        bool isArray{false};

        // Array size
        uint32_t arrayCount = 0;

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
            if (tinyxml2::XMLText* text = array->ToText()) {
                std::string trimmed = std::trim_copy(text->Value());

                if (!trimmed.empty() && trimmed[0] == '[') {
                    isArray = true;

                    uint64_t end = trimmed.find(']');
                    if (end == std::string::npos) {
                        std::cerr << "Malformed type in line: " << memberNode->GetLineNum() << ", array length end not found" << std::endl;
                        return false;
                    }

                    std::string length = trimmed.substr(1, end - 1);
                    arrayCount = std::atoi(length.c_str());
                }
            }
        }

        // Comments
        state.deepCopy << "\n" << Pad(indent) << "// " << sourceAccessorPrefix << memberName->GetText() << "\n";

        // Indirection?
        if (isIndirection) {
            // Get the length, try alt-len first
            const char* length = memberNode->Attribute("altlen", nullptr);

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
                    state.byteSize << Pad(indent) << "uint64_t " << sizeVar << " = std::strlen(" << sourceAccessorPrefix << memberName->GetText() << ");\n";

                    // Repeat for deep copy if scope requires it
                    if (indent > 1) {
                        state.deepCopy << Pad(indent) << "uint64_t " << sizeVar << " = std::strlen(" << sourceAccessorPrefix << memberName->GetText() << ");\n";
                    }

                    // Byte size
                    state.byteSize << Pad(indent) << "blobSize += sizeof(char) * " << sizeVar << ";\n";

                    // Assign the indirection
                    std::string mutableName = AssignPtrAndGetMutable(state, destAccessorPrefix, memberType, memberName, indent);

                    // Copy
                    state.deepCopy << Pad(indent) << "std::memcpy(" << mutableName << ", &" << sourceAccessorPrefix << memberName->GetText();
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
                    if (indent > 1) {
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
                        state.deepCopy << Pad(indent) << "std::memcpy(" << mutableName << ", " << "&" << sourceAccessorPrefix << memberName->GetText();
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
                        if (!DeepCopyObjectTree(md, state, elementType->second, sourceAccessorPrefix + memberName->GetText() + "[" + counterVar + "].", mutableName + "[" + counterVar + "].", indent + 1)) {
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
                    state.deepCopy << Pad(indent) << "std::memcpy(, " << mutableName << ", &" << sourceAccessorPrefix << memberName->GetText();
                    state.deepCopy << ", sizeof(" << sizeType <<  "));\n";
                } else {
                    // Copy the tree
                    if (!DeepCopyObjectTree(md, state, elementType->second, sourceAccessorPrefix + memberName->GetText() + "->", mutableName + "->", indent)) {
                        return false;
                    }
                }
            }
        } else if (isArray) {
            // POD array copy
            state.deepCopy << Pad(indent) << "std::memcpy(" << destAccessorPrefix << memberName->GetText() << ", " << sourceAccessorPrefix << memberName->GetText();
            state.deepCopy << ", sizeof(" << sourceAccessorPrefix + memberName->GetText() <<  "));\n";
        } else {
            // POD copy
            state.deepCopy << Pad(indent) << destAccessorPrefix << memberName->GetText() << " = " << sourceAccessorPrefix << memberName->GetText() << ";\n";
        }
    }

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
        deepCopy << "\tblob = new (allocators) uint8_t[blobSize];\n";

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

    // Instantiate template
    if (!templateEngine.Substitute("$DEEPCOPY", deepCopy.str().c_str())) {
        std::cerr << "Bad template, failed to substitute $DEEPCOPY" << std::endl;
        return false;
    }

    // OK
    return true;
}
