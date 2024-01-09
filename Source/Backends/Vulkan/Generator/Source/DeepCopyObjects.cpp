// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
        objects << "\t" << name << "DeepCopy() = default;\n";

        // Destructor
        objects << "\t~" << name << "DeepCopy();\n\n";

        // Deep copy constructor
        objects << "\tvoid DeepCopy(const Allocators& allocators, const " << name << "& source, bool copyExtensionStructures = true);\n\n";

        // Deep copy accessor
        objects << "\t" << name << "* operator->() {\n";
        objects << "\t\treturn &createInfo;\n";
        objects << "\t}\n\n";
        
        // Deep copy accessor
        objects << "\tconst " << name << "* operator->() const {\n";
        objects << "\t\treturn &createInfo;\n";
        objects << "\t}\n\n";

        // Creation info
        objects << "\t" << name << " createInfo{};\n";

        // Allocators
        objects << "\tAllocators allocators;\n";

        // Indirection blob
        objects << "\tuint8_t* blob{nullptr};\n";

        // Indirection size
        objects << "\tuint64_t length{0u};\n";

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
