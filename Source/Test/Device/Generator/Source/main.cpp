// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

// Std
#include <iostream>
#include <filesystem>
#include <sstream>
#include <fstream>

// Argparse
#include <argparse/argparse.hpp>

// Common
#include <Common/TemplateEngine.h>

// Generator
#include "Parser.h"
#include "Assembler.h"

int main(int argc, char *const argv[]) {
    argparse::ArgumentParser argParser("GPUOpen GRS - TestSuite Backend generator");

    // Setup parameters
    argParser.add_argument("-shaderType").help("Shader type file").default_value(std::string(""));
    argParser.add_argument("-test").help("Test file").default_value(std::string(""));
    argParser.add_argument("-name").help("Shader name").default_value(std::string(""));
    argParser.add_argument("-templates").help("Path to the templates").default_value(std::string(""));
    argParser.add_argument("-shader").help("Path of the generated shader header").default_value(std::string(""));
    argParser.add_argument("-feature").help("Name of the feature").default_value(std::string(""));
    argParser.add_argument("-o").help("Output of the generated file").default_value(std::string(""));

    // Attempt to parse the input
    try {
        argParser.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << argParser;
        return 1;
    }

    // Arguments
    auto &&shaderType = argParser.get<std::string>("-shaderType");
    auto &&test = argParser.get<std::string>("-test");
    auto &&name = argParser.get<std::string>("-name");
    auto &&templates = argParser.get<std::string>("-templates");
    auto &&shader = argParser.get<std::string>("-shader");
    auto &&feature = argParser.get<std::string>("-feature");
    auto &&out = argParser.get<std::string>("-o");

    // Mode
    if (!shaderType.empty()) {
        TemplateEngine shaderTypeTemplate;

        // Load template
        if (!shaderTypeTemplate.Load((std::filesystem::path(templates) / "ShaderType.cppt").string().c_str())) {
            std::cerr << "Failed to load template" << std::endl;
            return 1;
        }

        // Names
        shaderTypeTemplate.SubstituteAll("$PATH", shaderType.c_str());
        shaderTypeTemplate.SubstituteAll("$NAME", name.c_str());

        // Attempt to open the output
        std::ofstream outFile(out);
        if (!outFile.good()) {
            std::cerr << "Failed to open file '" << out << "'" << std::endl;
            return 1;
        }

        // Write contents
        outFile << shaderTypeTemplate.GetString();
    } else if (!test.empty()) {
        // Attempt to load the test
        std::ifstream testFile(test);
        if (!testFile.good()) {
            std::cerr << "Failed to open file '" << test << "'" << std::endl;
            return 1;
        }

        // To string
        std::string testSource((std::istreambuf_iterator<char>(testFile)), std::istreambuf_iterator<char>());

        // Destination program
        Program program;

        // Attempt to parse the program
        Parser parser(program);
        if (!parser.Parse(testSource.c_str())) {
            std::cerr << "Parser failed" << std::endl;
            return 1;
        }

        // Attempt to open the output
        std::ofstream outFile(out);
        if (!outFile.good()) {
            std::cerr << "Failed to open file '" << out << "'" << std::endl;
            return 1;
        }

        // Name of the program
        std::string programName = std::filesystem::path(test).stem().string();

        // Assembling info
        AssembleInfo assembleInfo{};
        assembleInfo.templatePath = templates.c_str();
        assembleInfo.shaderPath = shader.c_str();
        assembleInfo.program = programName.c_str();
        assembleInfo.feature = feature.c_str();

        // Assemble the final program
        Assembler assembler(assembleInfo, program);
        if (!assembler.Assemble(outFile)) {
            std::cerr << "Failed to assemble file '" << out << "'" << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Invalid usage" << std::endl;
        return 1;
    }

    // OK
    return 0;
}