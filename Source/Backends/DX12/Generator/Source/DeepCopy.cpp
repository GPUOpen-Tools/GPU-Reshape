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
        ss << "\t\tASSERT(valid, \"Object not created\");\n";
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

        // Copy validity
        ss << "\tbool valid{false};\n";

        // End object
        ss << "};\n\n";
    }

    // Handle all serializers
    for (auto&& object : info.deepCopy["serializers"]) {
        std::string name = object.get<std::string>();
        ss << "size_t Serialize(const " << name << "& source, " << name << "& dest, void* blob);\n";
    }

    // Instantiate template
    if (!templateEngine.Substitute("$OBJECTS", ss.str().c_str())) {
        std::cerr << "Bad template, failed to substitute $OBJECTS" << std::endl;
        return false;
    }

    // OK
    return true;
}
