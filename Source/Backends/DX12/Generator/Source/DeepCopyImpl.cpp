#include "GenTypes.h"
#include "Types.h"
#include "Name.h"

// Std
#include <iostream>
#include <array>

/// State of a given deep copy
struct DeepCopyState {
    /// Current local counter
    uint32_t counter{0};

    /// Streams
    std::stringstream byteSize;
    std::stringstream deepCopy;
};

/// Pad a string
static std::string Pad(uint32_t n) {
    return std::string(n, '\t');
}

/// Assign an indirection to the creation state, and create a local mutable representation
/// \param state the current state
/// \param accessorPrefix destination accessor prefix
/// \param memberType type of the member
/// \param memberName name of the member
/// \param indent current indentation level
/// \return the mutable variable name
std::string AssignPtrAndGetMutable(DeepCopyState &state, const std::string &accessorPrefix, const nlohmann::json& memberType, const std::string& memberName, uint32_t indent) {
    std::string mutableName = "mutable" + std::to_string(state.counter++);

    // Create mutable
    state.deepCopy << Pad(indent) << "auto* " << mutableName << " = reinterpret_cast<";
    PrettyPrintType(state.deepCopy, memberType, false, false);
    state.deepCopy << "*>";
    state.deepCopy << "(&blob[blobOffset]);\n";

    // Assign it
    state.deepCopy << Pad(indent) << accessorPrefix << memberName << " = " << mutableName << ";\n";

    // OK
    return mutableName;
}

/// Is the typename a com type?
/// \param info generator info
/// \param name typename
/// \return true if com type
static bool IsComType(const GeneratorInfo& info, const std::string& name) {
    return info.specification["interfaces"].contains(name) && info.specification["interfaces"][name].contains("vtable");
}

/// Is the typename a pod type?
/// \param info generator info
/// \param name typename
/// \return true if pod type
static bool IsPOD(const GeneratorInfo& info, const std::string& name) {
    return !info.specification["structs"].contains(name);
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
static bool DeepCopyObjectTree(const GeneratorInfo& info, DeepCopyState &state, const std::string& typeName, const std::string &sourceAccessorPrefix = "source.", const std::string &destAccessorPrefix = "desc.", uint32_t indent = 1) {
    auto&& type = info.specification["structs"][typeName];

    // Go through all members
    for (auto&& field : type["fields"]) {
        auto&& memberType = field["type"];

        // Get the name
        std::string memberName = field["name"].get<std::string>();

        // Comments
        state.deepCopy << "\n" << Pad(indent) << "// " << sourceAccessorPrefix << memberName << "\n";

        // Indirection?
        if (memberType["type"] == "pointer") {
            auto&& contained = memberType["contained"];

            // Wrap in check
            // Note that SAL does provide optional hints, however I do not trust them
            state.byteSize << Pad(indent) << "if (" << sourceAccessorPrefix + memberName << ") {\n";
            state.deepCopy << Pad(indent) << "if (" << sourceAccessorPrefix + memberName << ") {\n";
            indent++;

            // Attributed specials
            std::string byteSizeName;
            std::string sizeName;

            // Specialized?
            if (auto attr = field.find("attributes"); attr != field.end()) {
                if (auto byteSize = attr->find("byteSize"); byteSize != attr->end()) {
                    byteSizeName = (*byteSize)[0].get<std::string>();
                } else if (auto size = attr->find("size"); size != attr->end()) {
                    sizeName = (*size)[0].get<std::string>();
                }
            }

            // Provided a total byte size?
            if (byteSizeName.length()) {
                // Size variable
                std::string sizeVar = "size_" + std::to_string(state.counter++);

                // Accessor to the length (member)
                std::string length = sourceAccessorPrefix + byteSizeName;

                // Emit size var
                state.byteSize << Pad(indent) << "uint64_t " << sizeVar << " = " << length << ";\n";

                // Repeat for deep copy if scope requires it
                if (indent > 1) {
                    state.deepCopy << Pad(indent) << "uint64_t " << sizeVar << " = " << length << ";\n";
                }

                // Emit byte size
                state.byteSize << Pad(indent) << "blobSize += sizeof(uint8_t) * " << sizeVar << ";\n";

                // Assign the indirection
                std::string mutableName = AssignPtrAndGetMutable(state, destAccessorPrefix, contained, memberName, indent);

                state.deepCopy << Pad(indent) << "std::memcpy(" << mutableName << ", " << "" << sourceAccessorPrefix << memberName;
                state.deepCopy << ", sizeof(uint8_t) * " << sizeVar << ");\n";

                // Offset
                state.deepCopy << Pad(indent) << "blobOffset += sizeof(uint8_t) * " << sizeVar << ";\n";
            }

            // Provided a (typed) array length?
            else if (sizeName.length()) {
                // Size variable
                std::string sizeVar = "size_" + std::to_string(state.counter++);

                // Accessor to the length (member)
                std::string length = sourceAccessorPrefix + sizeName;

                // Emit size var
                state.byteSize << Pad(indent) << "uint64_t " << sizeVar << " = " << length << ";\n";

                // Repeat for deep copy if scope requires it
                if (indent > 1) {
                    state.deepCopy << Pad(indent) << "uint64_t " << sizeVar << " = " << length << ";\n";
                }

                // Standard pointer byte size
                std::string sizeType;
                if (contained["type"] ==  "void") {
                    sizeType = "uint8_t";
                } else {
                    sizeType = "*" + sourceAccessorPrefix + memberName;
                }

                // Emit byte size
                state.byteSize << Pad(indent) << "blobSize += sizeof(" << sizeType << ") * " << sizeVar << ";\n";

                // Assign the indirection
                std::string mutableName = AssignPtrAndGetMutable(state, destAccessorPrefix, contained, memberName, indent);

                // Name of the contained type
                std::string containedName = contained["name"].get<std::string>();

                // POD types are just memcpy'ied
                if (IsPOD(info, containedName)) {
                    state.deepCopy << Pad(indent) << "std::memcpy(" << mutableName << ", " << "" << sourceAccessorPrefix << memberName;
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
                    if (!DeepCopyObjectTree(info, state, containedName, sourceAccessorPrefix + memberName + "[" + counterVar + "].", mutableName + "[" + counterVar + "].", indent + 1)) {
                        return false;
                    }

                    // End the loops
                    state.byteSize << Pad(indent) << "}\n";
                    state.deepCopy << Pad(indent) << "}\n";
                }
            }

            // Com type?
            else if (contained.contains("name") && IsComType(info, contained["name"])) {
                // Just assign it directly, parent code handles lifetime
                state.deepCopy << Pad(indent) << destAccessorPrefix << memberName << " = " << sourceAccessorPrefix << memberName << ";\n";
            }

            // Standard indirection
            else {
                // Standard pointer byte size
                std::string sizeType;
                if (contained["type"] ==  "void") {
                    sizeType = "uint8_t";
                } else {
                    sizeType = "*" + sourceAccessorPrefix + memberName;
                }

                // Standard pointer byte size
                state.byteSize << Pad(indent) << "blobSize += sizeof(" << sizeType << ");\n";

                // Assign the indirection
                std::string mutableName = AssignPtrAndGetMutable(state, destAccessorPrefix, contained, memberName, indent);

                // Offset
                state.deepCopy << Pad(indent) << "blobOffset += sizeof(" << sizeType << ");\n";

                // Name of the contained type
                std::string containedName = contained["name"].get<std::string>();

                // POD types are just memcpy'ied
                if (IsPOD(info, containedName)) {
                    state.deepCopy << Pad(indent) << "std::memcpy(" << mutableName << ", " << sourceAccessorPrefix << memberName;
                    state.deepCopy << ", sizeof(" << sizeType << "));\n";
                } else {
                    // Copy the tree
                    if (!DeepCopyObjectTree(info, state, containedName, sourceAccessorPrefix + memberName + "->", mutableName + "->", indent)) {
                        return false;
                    }
                }
            }

            indent--;

            state.byteSize << Pad(indent) << "}\n";

            // If not specified, set mutable state to nullptr
            state.deepCopy << Pad(indent) << "} else {\n";
            state.deepCopy << Pad(indent + 1) << destAccessorPrefix << memberName << " = nullptr;\n";
            state.deepCopy << Pad(indent) << "}\n";
        } else if (memberType["type"] == "array") {
            auto&& contained = memberType["contained"];

            // Name of the contained type
            std::string containedName = contained["name"].get<std::string>();

            // POD types are just memcpy'ied
            if (IsPOD(info, containedName)) {
                // POD array copy
                state.deepCopy << Pad(indent) << "std::memcpy(" << destAccessorPrefix << memberName << ", " << sourceAccessorPrefix << memberName;
                state.deepCopy << ", sizeof(" << sourceAccessorPrefix + memberName << "));\n";
            } else {
                // Counter var
                std::string counterVar = "i" + std::to_string(state.counter++);
                state.deepCopy << Pad(indent) << "for (size_t " << counterVar << " = 0; " << counterVar << " < " << memberType["size"] << "; " << counterVar << "++) {\n";

                // Copy the tree
                if (!DeepCopyObjectTree(
                    info, state, containedName,
                    sourceAccessorPrefix + memberName + "[" + counterVar + "].",
                    destAccessorPrefix + memberName + "[" + counterVar + "].",
                    indent + 1
                )) {
                    return false;
                }

                state.deepCopy << Pad(indent) << "}\n";
            }
        } else {
            // Name of the contained type
            std::string containedName = memberType["name"].get<std::string>();

            // If end, this is a POD type
            if (IsPOD(info, containedName)) {
                state.deepCopy << Pad(indent) << destAccessorPrefix << memberName << " = " << sourceAccessorPrefix << memberName << ";\n";
            } else {
                // Copy the tree
                if (!DeepCopyObjectTree(info, state, containedName, sourceAccessorPrefix + memberName + ".", destAccessorPrefix + memberName + ".", indent)) {
                    return false;
                }
            }
        }
    }

    // OK
    return true;
}

bool Generators::DeepCopyImpl(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    // Final stream
    std::stringstream deepCopy;

    // Create deep copies
    for (auto&& object : info.deepCopy["objects"]) {
        std::string name = object.get<std::string>();

        // Get the friendly deep copy name
        std::string copyName = GetPrettyName(name) + "DeepCopy";

        // Attempt to generate a deep copy
        DeepCopyState state;
        if (!DeepCopyObjectTree(info, state, name)) {
            return false;
        }

        // Begin deep copy constructor
        deepCopy << "void " << copyName << "::DeepCopy(const Allocators& _allocators, const " << name << "& source) {\n";
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
        deepCopy << copyName << "::~" << copyName << "() {\n";

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