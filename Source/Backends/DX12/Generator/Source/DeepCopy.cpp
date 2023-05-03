#include "GenTypes.h"
#include "Name.h"

// Std
#include <iostream>
#include <array>

bool Generators::DeepCopy(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    // Final stream
    std::stringstream ss;

    // Handle all objects
    for (auto&& object : info.deepCopy["objects"]) {
        std::string name = object.get<std::string>();

        std::string copyName = GetPrettyName(name) + "DeepCopy";

        // Begin object
        ss << "struct " << copyName << " {\n";

        // Default constructor
        ss << "\t" << copyName << "() = default;\n";

        // Destructor
        ss << "\t~" << copyName << "();\n\n";

        // Deep copy constructor
        ss << "\tvoid DeepCopy(const Allocators& allocators, const " << name << "& source);\n\n";

        // Deep copy accessor
        ss << "\t" << name << "* operator->() {\n";
        ss << "\t\treturn &desc;\n";
        ss << "\t}\n\n";

        // Creation info
        ss << "\t" << name << " desc{};\n";

        // Allocators
        ss << "\tAllocators allocators;\n";

        // Indirection blob
        ss << "\tuint8_t* blob{nullptr};\n";

        // Indirection size
        ss << "\tuint64_t length{0u};\n";

        // End object
        ss << "};\n\n";
    }

    // Instantiate template
    if (!templateEngine.Substitute("$OBJECTS", ss.str().c_str())) {
        std::cerr << "Bad template, failed to substitute $OBJECTS" << std::endl;
        return false;
    }

    // OK
    return true;
}
