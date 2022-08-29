#include "GenTypes.h"
#include "Types.h"
#include "Name.h"

// Common
#include <Common/String.h>

// Std
#include <sstream>
#include <regex>
#include <string_view>

namespace {
    struct ParameterInfo {
        std::string name;
        std::string info;
    };
}

/// Translate a given RST type into spec type
/// \param type RST type
/// \return empty if failed
static std::string TranslateType(const std::string_view& type) {
    if (type == "void") {
        return "DXILIntrinsicTypeSpec::Void";
    } else if (type == "i32") {
        return "DXILIntrinsicTypeSpec::I32";
    } else if (type == "float") {
        return "DXILIntrinsicTypeSpec::F32";
    } else if (type == "i8") {
        return "DXILIntrinsicTypeSpec::I8";
    } else if (type == "i1") {
        return "DXILIntrinsicTypeSpec::I1";
    } else if (type == "%dx.types.Handle") {
        return "DXILIntrinsicTypeSpec::Handle";
    }

    // Unknown
    return "";
}

bool Generators::DXILIntrinsics(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    std::stringstream intrinsics;

    // Current UID
    uint32_t uid = 0;

    // Regex patterns
    std::regex declarePattern("declare (%?[A-Za-z.0-9]+) @([A-Za-z.0-9]+)\\(");
    std::regex parameterPattern("\\s*(%?[A-Za-z0-9\\.]+)(,|\\))(\\s+; (.*))?$");

    // For all declarations
    for(std::sregex_iterator m = std::sregex_iterator(info.dxilRST.begin(), info.dxilRST.end(), declarePattern); m != std::sregex_iterator(); m++) {
        // Translate return type
        std::string returnType = TranslateType((*m)[1].str());
        if (returnType.empty()) {
            continue;
        }

        // Get name
        std::string name = (*m)[2].str();

        // Convert key wise name
        std::string keyName;
        for (size_t i = 0; i < name.length(); i++) {
            if (name[i] == '.') {
                continue;
            }

            // Capitalize first character and after '.'
            keyName.push_back((i == 0 || name[i - 1] == '.') ? std::toupper(name[i]) : name[i]);
        }

        // Any error?
        bool wasError = false;

        // All parameters
        std::vector<ParameterInfo> parameters;

        // For all parameters
        for(std::sregex_iterator p = std::sregex_iterator(info.dxilRST.begin() + m->position() + m->length(), info.dxilRST.end(), parameterPattern); p != std::sregex_iterator(); p++) {
            // Translate parameter type
            std::string paramType = TranslateType((*p)[1].str());
            if (paramType.empty()) {
                wasError = true;
                break;
            }

            // Add parameter
            parameters.push_back(ParameterInfo {
                .name = paramType,
                .info = (*p)[4].str()
            });

            // End of intrinsic?
            if ((*p)[2].str() == ")") {
                break;
            }
        }

        // Failed parsing?
        if (wasError) {
            continue;
        }

        // Emit intrinsic spec
        intrinsics << "\tstatic DXILIntrinsicSpec " << keyName << " {\n";
        intrinsics << "\t\t.uid = " << (uid++) << ",\n";
        intrinsics << "\t\t.name = \"" << name << "\",\n";
        intrinsics << "\t\t.returnType = " << returnType << ",\n";
        intrinsics << "\t\t.parameterTypes = {\n";

        // Emit parameters
        for (const ParameterInfo& param : parameters) {
            intrinsics << "\t\t\t" << param.name << ", // " << param.info << "\n";
        }

        // Close
        intrinsics << "\t\t}\n";
        intrinsics << "\t};\n\n";
    }

    // Substitute keys
    templateEngine.Substitute("$INTRINSICS", intrinsics.str().c_str());

    // OK
    return true;
}
