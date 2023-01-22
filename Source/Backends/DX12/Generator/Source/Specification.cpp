#include "GenTypes.h"

// libClang
#include <clang-c/Index.h>

// Common
#include <Common/FileSystem.h>
#include <Common/String.h>

// Std
#include <vector>
#include <iostream>

/// Specification state
struct SpecificationState {
    // Buckets
    nlohmann::json structs;
    nlohmann::json interfaces;
};

/// Visitation helper
/// \param cursor top level cursor to visit
/// \param functor lambda
template<typename F>
static void VisitCursor(CXCursor cursor, F &&functor) {
    clang_visitChildren(cursor, [](CXCursor cursor, CXCursor parent, CXClientData clientData) -> CXChildVisitResult {
        return static_cast<F *>(clientData)->operator()(cursor);
    }, &functor);
}

/// Translate a clang type to json type
static nlohmann::json TranslateType(CXType type) {
    nlohmann::json obj;
    obj["const"] = clang_isConstQualifiedType(type);

    // Handle kind
    switch (type.kind) {
        default: {
            CXCursor declaration = clang_getTypeDeclaration(type);

            // Get canonical type
            CXCursor underlyingDeclaration = declaration;
            if (underlyingDeclaration.kind == CXCursor_TypedefDecl) {
                underlyingDeclaration = clang_getTypeDeclaration(clang_getTypedefDeclUnderlyingType(declaration));
            }

            // Is the declaration structural?
            if (underlyingDeclaration.kind == CXCursor_StructDecl) {
                obj["type"] = "struct";
            } else {
                obj["type"] = "pod";
            }

            if (clang_equalCursors(declaration, clang_getNullCursor())) {
                obj["name"] = clang_getCString(clang_getTypeSpelling(type));
            } else {
                obj["name"] = clang_getCString(clang_getCursorSpelling(declaration));
            }
            break;
        }
        case CXType_Void: {
            obj["type"] = "void";
            obj["name"] = "void";
            break;
        }
        case CXType_Pointer: {
            obj["type"] = "pointer";
            obj["contained"] = TranslateType(clang_getPointeeType(type));
            break;
        }
        case CXType_LValueReference: {
            obj["type"] = "lref";
            obj["contained"] = TranslateType(clang_getPointeeType(type));
            break;
        }
        case CXType_RValueReference: {
            obj["type"] = "rref";
            obj["contained"] = TranslateType(clang_getPointeeType(type));
            break;
        }
        case CXType_ConstantArray: {
            obj["type"] = "array";
            obj["size"] = clang_getArraySize(type);
            obj["contained"] = TranslateType(clang_getArrayElementType(type));
            break;
        }
        case CXType_FunctionProto: {
            obj["type"] = "function";
            obj["returnType"] = TranslateType(clang_getResultType(type));

            // Append parameters
            for (int i = 0; i < clang_getNumArgTypes(type); i++) {
                obj["parameters"].emplace_back(TranslateType(clang_getArgType(type, i)));
            }
            break;
        }
    }

    return obj;
}

/// Reverse search to the start of an attribute
/// \param ptr starting ptr
/// \param offset current offset
/// \return the position within the ptr
const char* ReverseSearchAttribute(const char* ptr, uint32_t offset) {
    // Eat whitespaces
    while (offset && std::iswhitespace(ptr[offset])) {
        offset--;
    }

    // Valid attribute must end with ')'
    if (!offset || ptr[offset] != ')') {
        return nullptr;
    }

    // Eat ')'
    offset--;

    // Current bracket pop counter
    uint32_t popCounter = 1;

    // Eat *all* characters until pop counter has been satisfied
    while (offset && popCounter) {
        popCounter += ptr[offset] == ')';
        popCounter -= ptr[offset] == '(';
        offset--;
    }

    // Eat whitespaces
    while (offset && std::iswhitespace(ptr[offset])) {
        offset--;
    }

    // Eat the type of attribute
    while (offset && std::iscxxalnum(ptr[offset])) {
        offset--;
    }

    // Start of attribute
    return ptr + offset + 1;
}

/// Try to parse an attribute into a field
/// \param unit the translation unit
/// \param cursor declaration cursor
/// \param field destination field
void TryParseAttributes(CXTranslationUnit unit, CXCursor cursor, nlohmann::json &field) {
    // Get the cursor range
    CXSourceRange range = clang_getCursorExtent(cursor);

    // Source wise offset
    uint32_t offset;

    // Get the file location
    CXFile file;
    clang_getFileLocation(clang_getRangeStart(range), &file, nullptr, nullptr, &offset);

    // Get (cached) file contents
    size_t len;
    const char *contents = clang_getFileContents(unit, file, &len);
    if (!contents) {
        return;
    }

    // Find start of attribute
    const char* ptr = ReverseSearchAttribute(contents, offset - 1);
    if (!ptr) {
        return;
    }

    /*
     * Note
     *  ! This is not a good parser, it's just *enough* for the use case. If need be something more intelligent can be written.
     * */

    // Eat the type of attribute
    const char* startName = ptr;
    while (std::iscxxalnum(*ptr)) {
        ptr++;
    }

    // Translate type
    std::string type(startName, ptr);
    if (type == "_Field_size_bytes_full_" || type == "_Field_size_bytes_full_opt_") {
        type = "byteSize";
    } else if (type == "_Field_size_full_" || type == "_In_reads_") {
        type = "size";
    }

    // Eat whitespaces
    while (std::iswhitespace(*ptr)) {
        ptr++;
    }

    // Must be '('
    if (*(ptr++) != '(') {
        return;
    }

    // Parse arguments (assumes single attribute)
    nlohmann::json args;
    for (;;) {
        // Eat whitespaces
        while (std::iswhitespace(*ptr)) {
            ptr++;
        }

        // Eat the argument
        const char* argName = ptr;
        while (std::iscxxalnum(*ptr)) {
            ptr++;
        }

        // Append argument
        args.push_back(std::string(argName, ptr));

        // Eat whitespaces
        while (std::iswhitespace(*ptr)) {
            ptr++;
        }

        // Next argument?
        if (*ptr != ',') {
            break;
        }

        // Eat ','
        ptr++;
    }

    // Eat whitespaces
    while (std::iswhitespace(*ptr)) {
        ptr++;
    }

    // Must be '('
    if (*(ptr++) != ')') {
        return;
    }

    // Attribute object
    nlohmann::json attributes;
    attributes[type] = args;

    // Assign to field
    field["attributes"] = attributes;
}

/// Reflect a given class cursor
static bool ReflectClass(SpecificationState& state, CXTranslationUnit unit, CXCursor child) {
    // Ignore forward declarations
    //   Check invalid definitions and out-of-place definitions
    CXCursor definition = clang_getCursorDefinition(child);
    if (clang_equalCursors(definition, clang_getNullCursor()) || !clang_equalCursors(child, definition)) {
        return CXChildVisit_Continue;
    }

    // Get name
    std::string name = clang_getCString(clang_getCursorDisplayName(child));
    if (name.empty()) {
        return CXChildVisit_Continue;
    }

    nlohmann::json decl;

    // Buckets
    nlohmann::json fields;
    nlohmann::json bases;
    nlohmann::json vtable;
    nlohmann::json methods;

    // Visit all declaration cursors
    VisitCursor(child, [&](CXCursor declChild) {
        switch (clang_getCursorKind(declChild)) {
            default: {
                break;
            }
            case CXCursor_CXXBaseSpecifier: {
                std::string baseName = clang_getCString(clang_getCursorDisplayName(declChild));

                // Reflect the base classes even if originating outside the file whitelist
                if (!state.interfaces.contains(baseName)) {
                    // Get the resulting base class type, transform to the canonical cursor
                    CXCursor canonCursor = clang_getTypeDeclaration(clang_getCanonicalType(clang_getCursorType(declChild)));

                    // Reflect base class
                    ReflectClass(state, unit, canonCursor);
                }

                bases.emplace_back(baseName);
                break;
            }
            case CXCursor_FieldDecl: {
                CXType type = clang_getCursorType(declChild);
                std::string fieldName = clang_getCString(clang_getCursorDisplayName(declChild));

                nlohmann::json field;
                field["name"] = fieldName;
                field["type"] = TranslateType(type);

                // Check attributes
                TryParseAttributes(unit, declChild, field);

                // Emplace
                fields.emplace_back(field);
                break;
            }
            case CXCursor_CXXMethod: {
                std::string name = clang_getCString(clang_getCursorDisplayName(declChild));

                // Clean up name
                if (size_t end = name.find('('); end != std::string::npos) {
                    name = name.substr(0, end);
                }

                nlohmann::json method;
                method["name"] = name;
                method["returnType"] = TranslateType(clang_getCursorResultType(declChild));

                // Buckets
                nlohmann::json params;

                // Translate parameters
                VisitCursor(declChild, [&](CXCursor paramCursor) {
                    switch (paramCursor.kind) {
                        default: {
                            break;
                        }
                        case CXCursor_ParmDecl: {
                            nlohmann::json param;
                            param["name"] = std::string(clang_getCString(clang_getCursorDisplayName(paramCursor)));
                            param["type"] = TranslateType(clang_getCursorType(paramCursor));
                            params.push_back(param);
                            break;
                        }
                    }
                    return CXChildVisit_Continue;
                });

                // Append buckets
                method["params"] = params;

                // Emplace to virtual table if need be
                if (clang_CXXMethod_isVirtual(declChild) || clang_CXXMethod_isPureVirtual(declChild)) {
                    vtable.emplace_back(method);
                } else {
                    methods.emplace_back(method);
                }
                break;
            }
        }
        return CXChildVisit_Continue;
    });

    // Create object
    decl["fields"] = fields;
    decl["methods"] = methods;
    decl["bases"] = bases;
    decl["vtable"] = vtable;

    // Append to appropriate bucket
    if (name[0] == 'I') {
        state.interfaces[name] = decl;
    } else {
        state.structs[name] = decl;
    }

    // OK
    return true;
}

/// Specification generator
bool Generators::Specification(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    std::vector<const char *> args {
        // C++ language specifier
        "-x", "c++",
    };

    // Create index
    CXIndex index = clang_createIndex(false, true);
    if (!index) {
        return false;
    }

    CXTranslationUnit unit{nullptr};

    // Attempt to parse the header
    if (clang_parseTranslationUnit2(
        index,
        info.d3d12HeaderPath.c_str(),
        args.data(),
        static_cast<int>(args.size()),
        nullptr, 0,
        CXTranslationUnit_SkipFunctionBodies,
        &unit
    ) != CXError_Success) {
        // Cleanup unit
        clang_disposeIndex(index);

        // Error message
        std::cerr << "Failed to parse d3d12.h" << std::endl;
        return false;
    }

    // Display diagnostics
    for (uint32_t i = 0; i < clang_getNumDiagnostics(unit); i++) {
        std::cerr << clang_getCString(clang_formatDiagnostic(clang_getDiagnostic(unit, i), CXDiagnostic_DisplaySourceLocation)) << "\n";
    }

    SpecificationState state;


    // Visit all top cursors
    VisitCursor(clang_getTranslationUnitCursor(unit), [&](CXCursor child) {
        CXFile file;

        // Get the location
        clang_getExpansionLocation(clang_getCursorLocation(child), &file, nullptr, nullptr, nullptr);

        // As path
        std::filesystem::path filename = clang_getCString(clang_getFileName(file));

        bool relevant = false;

        // Ignore all files not form sources
        for (auto &&specFile: info.hooks["files"]) {
            if (specFile.get<std::string>() == filename.filename()) {
                relevant = true;
            }
        }

        // Not relevant?
        if (!relevant) {
            return CXChildVisit_Continue;
        }

        // Handle cursor
        switch (clang_getCursorKind(child)) {
            default: {
                // Recurse nested types
                return CXChildVisit_Recurse;
            }
            case CXCursor_StructDecl:
            case CXCursor_ClassDecl: {
                ReflectClass(state, unit, child);
                break;
            }
        }
        return CXChildVisit_Continue;
    });

    // Create root json
    nlohmann::json json;
    json["structs"] = state.structs;
    json["interfaces"] = state.interfaces;

    // Replace keys
    templateEngine.Substitute("$JSON", json.dump(2).c_str());

    // Cleanup
    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);

    // OK
    return true;
}
