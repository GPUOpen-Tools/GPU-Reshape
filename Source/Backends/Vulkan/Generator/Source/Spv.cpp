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
    std::stringstream operandStream;

    // Add case for all instructions of interest
    for (auto &&instruction: info.spvJson["instructions"]) {
        std::string _class = instruction["class"];

        // If not mapped or already covered, ignore
        if (!classMappings.count(_class) || coverage.count(instruction["opcode"])) {
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

    // Cleanup last coverage
    coverage.clear();

    // Add case for all instructions of interest
    for (auto &&instruction: info.spvJson["instructions"]) {
        std::string _class = instruction["class"];

        // If already covered, ignore
        if (coverage.count(instruction["opcode"])) {
            continue;
        }

        // Mapped
        coverage.insert(static_cast<uint32_t>(instruction["opcode"]));

        // Get name
        std::string opName = instruction["opname"].get<std::string>();

        // Ignore instruction coding
        uint32_t offset = 1;

        // No operands? Skip
        if (!instruction.contains("operands")) {
            continue;
        }

        // Open case
        operandStream << "\t\tcase Spv" << opName << ":\n";

        // Visit all id references that are bounded
        for (auto &&operand: instruction["operands"]) {
            if (operand["kind"] == "IdRef" && !operand.contains("quantifier")) {
                operandStream << "\t\t\tfunctor(words[" << (offset) << "]);\n";
            }

            offset++;
        }

        // Close case
        operandStream << "\t\t\tbreak;\n";
    }

    // Replace
    templateEngine.Substitute("$CLASSES", classStream.str().c_str());
    templateEngine.Substitute("$OPERANDS", operandStream.str().c_str());

    // OK
    return true;
}
