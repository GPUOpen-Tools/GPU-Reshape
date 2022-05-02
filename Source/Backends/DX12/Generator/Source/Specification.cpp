#include "GenTypes.h"

#include <clang-c/Index.h>

#include <Common/FileSystem.h>

#include <vector>
#include <iostream>
#include <sstream>

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

            obj["type"] = "pod";

            if (clang_equalCursors(declaration, clang_getNullCursor())) {
                obj["name"] = clang_getCString(clang_getTypeSpelling(type));
            } else {
                obj["name"] = clang_getCString(clang_getCursorSpelling(declaration));
            }
            break;
        }
        case CXType_Void: {
            obj["type"] = "void";
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
            for (uint32_t i = 0; i < clang_getNumArgTypes(type); i++) {
                obj["parameters"].emplace_back(TranslateType(clang_getArgType(type, i)));
            }
            break;
        }
    }

    return obj;
}

/// Specification generator
bool Generators::Specification(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    std::vector<const char *> args;

    // Create index
    CXIndex index = clang_createIndex(false, false);
    if (!index) {
        return false;
    }

    CXTranslationUnit unit{nullptr};

    // Attempt to parse the header
    if (clang_parseTranslationUnit2(
        index,
        info.d3d12HeaderPath.c_str(),
        args.data(),
        args.size(),
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

    // Buckets
    nlohmann::json structs;
    nlohmann::json interfaces;
    nlohmann::json vtables;

    // Visit all top cursors
    VisitCursor(clang_getTranslationUnitCursor(unit), [&](CXCursor child) {
        CXFile file;

        // Get the location
        clang_getExpansionLocation(clang_getCursorLocation(child), &file, nullptr, nullptr, nullptr);

        // As path
        std::filesystem::path filename = clang_getCString(clang_getFileName(file));

        bool relevant = false;

        // Ignore all files not form sources
        for (auto&& specFile : info.hooks["files"]) {
            if (specFile.get<std::string>() == filename.filename()) {
                relevant = true;
            }
        }

        // Handle cursor
        switch (clang_getCursorKind(child)) {
            default: {
                break;
            }
            case CXCursor_StructDecl: {
                // Ignore forward declarations
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

                // Visit all declaration cursors
                VisitCursor(child, [&](CXCursor declChild) {
                    switch (clang_getCursorKind(declChild)) {
                        default: {
                            break;
                        }
                        case CXCursor_CXXBaseSpecifier: {
                            std::string baseName = clang_getCString(clang_getCursorDisplayName(declChild));
                            bases.emplace_back(baseName);
                            break;
                        }
                        case CXCursor_FieldDecl: {
                            CXType type = clang_getCursorType(declChild);
                            std::string fieldName = clang_getCString(clang_getCursorDisplayName(declChild));

                            if (fieldName == "lpVtbl") {
                                CXCursor declaration = clang_getTypeDeclaration(clang_getPointeeType(type));
                                decl["vtable"] = clang_getCString(clang_getCursorSpelling(declaration));
                            } else {
                                nlohmann::json field;
                                field["name"] = fieldName;
                                field["type"] = TranslateType(type);
                                fields.emplace_back(field);
                            }
                            break;
                        }
                    }
                    return CXChildVisit_Continue;
                });

                // Create object
                decl["fields"] = fields;
                decl["bases"] = bases;

                // Append to appropriate bucket
                if (name[0] == 'I') {
                    interfaces[name] = decl;
                } else {
                    structs[name] = decl;
                }
                break;
            }
        }
        return CXChildVisit_Continue;
    });

    // Create root json
    nlohmann::json json;
    json["structs"] = structs;
    json["interfaces"] = interfaces;

    // Replace keys
    templateEngine.Substitute("$JSON", json.dump(2).c_str());

    // Cleanup
    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);

    // OK
    return true;
}
