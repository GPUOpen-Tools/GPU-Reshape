#include "GenTypes.h"

#include <iostream>
#include <array>
#include <map>


bool Generators::DeepCopyObjects(const GeneratorInfo& info, TemplateEngine& templateEngine) {
    // Get the types
    tinyxml2::XMLElement *types = info.registry->FirstChildElement("types");
    if (!types) {
        std::cerr << "Failed to find commands in registry" << std::endl;
        return false;
    }

    // Final stream
    std::stringstream objects;

    // Create deep copies
    for (tinyxml2::XMLElement *typeNode = types->FirstChildElement("type"); typeNode; typeNode = typeNode->NextSiblingElement("type")) {
        // Try to get the category, if not, not interested
        const char* category = typeNode->Attribute("category", nullptr);
        if (!category) {
            continue;
        }

        // Skip non-compound types
        if (std::strcmp(category, "struct") != 0) {
            continue;
        }

        // Attempt to get the name
        const char* name = typeNode->Attribute("name", nullptr);
        if (!name) {
            std::cerr << "Malformed type in line: " << typeNode->GetLineNum() << ", name not found" << std::endl;
            return false;
        }

        // Candidate?
        if (!info.objects.count(name)) {
            continue;
        }

        // Begin object
        objects << "struct " << name << "DeepCopy {\n";

        // Default constructor
        objects << "\t" << name << "DeepCopy() = default;\n\n";

        // Destructor
        objects << "\t~" << name << "DeepCopy();\n\n";

        // Deep copy constructor
        objects << "\tvoid DeepCopy(const Allocators& allocators, const " << name << "& source);\n\n";

        // Creation info
        objects << "\t" << name << " createInfo{};\n";

        // Allocators
        objects << "\tAllocators allocators;\n";

        // Indirection blob
        objects << "\tuint8_t* blob{nullptr};\n";

        // End object
        objects << "};\n\n";
    }

    // Instantiate template
    if (!templateEngine.Substitute("$OBJECTS", objects.str().c_str())) {
        std::cerr << "Bad template, failed to substitute $OBJECTS" << std::endl;
        return false;
    }

    // OK
    return true;
}
