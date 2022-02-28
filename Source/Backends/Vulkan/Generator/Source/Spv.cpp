#include "GenTypes.h"

// Std
#include <iostream>
#include <map>
#include <set>

bool Generators::Spv(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    // Must have specification
    if (info.spvJson.empty()) {
        std::cerr << "Missing spv json file" << std::endl;
        return false;
    }

    // Class to physical type mapping
    static std::map<std::string, std::string> classMappings = {
            {"Miscellaneous", "TypeConstantVariable"},
            {"Annotation", "Annotation"},
            {"Type-Declaration", "TypeConstantVariable"},
            {"Constant-Creation", "TypeConstantVariable"},
    };

    // Covered instructions
    std::set<uint32_t> coverage;

    // Streams
    std::stringstream classStream;

    // Add case for all instructions of interest
    for (auto &&instruction: info.spvJson["instructions"]) {
        std::string _class = instruction["class"];

        // If not mapped or already covered, ignore
        if (!classMappings.contains(_class) || coverage.contains(instruction["opcode"])) {
            continue;
        }

        // Mapped
        coverage.insert(static_cast<uint32_t>(instruction["opcode"]));

        // Get name
        std::string opName = to_string(instruction["opname"]);

        // Generate case
        classStream << "\t\tcase Spv" << opName.substr(1, opName.length() - 2) << ":\n";
        classStream << "\t\t\treturn SpvPhysicalBlockType::" << classMappings.at(_class) << ";\n";
    }

    // Replace
    templateEngine.Substitute("$CLASSES", classStream.str().c_str());

    // OK
    return true;
}
