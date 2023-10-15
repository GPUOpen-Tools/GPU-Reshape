// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
#include <sstream>
#include <fstream>

// Argparse
#include <argparse/argparse.hpp>

// Xml
#include <tinyxml2.h>

// Common
#include <Common/TemplateEngine.h>

// Generator
#include "GenTypes.h"

int main(int argc, char *const argv[]) {
    argparse::ArgumentParser program("GPUOpen GRS - Vulkan Generator");

    // Setup parameters
    program.add_argument("-vkxml").help("Path of the vulkan specification xml file").required();
    program.add_argument("-template").help("The file to template").required();
    program.add_argument("-spvjson").help("The file to the spv json specification").default_value(std::string(""));
    program.add_argument("-gentype").help("The generation type, one of [commandbuffer, commandbufferdispatchtable, deepcopyobjects]").required();
    program.add_argument("-whitelist").help("Whitelist a callback").default_value(std::string(""));
    program.add_argument("-hook").help("All feature hooks").default_value(std::string(""));
    program.add_argument("-object").help("All generator objects").default_value(std::string(""));
    program.add_argument("-o").help("Output of the generated file").required();

    // Attempt to parse the input
    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    // Arguments
    auto &&vkxml = program.get<std::string>("-vkxml");
    auto &&gentype = program.get<std::string>("-gentype");
    auto &&ftemplate = program.get<std::string>("-template");
    auto &&spvjson = program.get<std::string>("-spvjson");
    auto &&output = program.get<std::string>("-o");
    auto &&whitelist = program.get<std::string>("-whitelist");
    auto &&object = program.get<std::string>("-object");
    auto &&hooks = program.get<std::string>("-hook");

    // Generator information
    GeneratorInfo generatorInfo{};

    if (!spvjson.empty()) {
        // Open json
        std::ifstream stream(spvjson);
        if (!stream.good()) {
            std::cerr << "Failed to open json file: " << spvjson << std::endl;
            return 1;
        }

        // Parse json
        try {
            stream >> generatorInfo.spvJson;
        } catch(nlohmann::json::exception& ex) {
            std::cerr << "Failed to parse json file: " << spvjson << ", " << ex.what() << std::endl;
            return 1;
        }
    }

    // Parse whitelist
    std::stringstream whitelistStream(whitelist);
    while( whitelistStream.good() ) {
        std::string substr;
        getline(whitelistStream, substr, ',' );
        generatorInfo.whitelist.insert(substr);
    }

    // Parse object
    std::stringstream objectStream(object);
    while( objectStream.good() ) {
        std::string substr;
        getline(objectStream, substr, ',' );
        generatorInfo.objects.insert(substr);
    }

    // Parse hooks
    std::stringstream hookStream(hooks);
    while( hookStream.good() ) {
        std::string substr;
        getline(hookStream, substr, ',' );
        generatorInfo.hooks.insert(substr);
    }

    // Attempt to open the specification xml
    tinyxml2::XMLDocument document;
    if (document.LoadFile(vkxml.c_str()) != tinyxml2::XML_SUCCESS) {
        std::cerr << "Failed to open vkxml file: " << vkxml << std::endl;
        return 1;
    }

    // Get the root registry
    generatorInfo.registry = document.FirstChildElement("registry");
    if (!generatorInfo.registry) {
        std::cerr << "Failed to find registry in specification" << std::endl;
        return 1;
    }

    // Try to open the template
    TemplateEngine templateEngine;
    if (!templateEngine.Load(ftemplate.c_str())) {
        std::cerr << "Failed to open template file: " << ftemplate << std::endl;
        return 1;
    }

    // Generator success
    bool generatorResult{};

    // Invoke generator
    if (gentype == "commandbuffer") {
        generatorResult = Generators::CommandBuffer(generatorInfo, templateEngine);
    } else if (gentype == "commandbufferdispatchtable") {
        generatorResult = Generators::CommandBufferDispatchTable(generatorInfo, templateEngine);
    } else if (gentype == "deepcopyobjects") {
        generatorResult = Generators::DeepCopyObjects(generatorInfo, templateEngine);
    } else if (gentype == "deepcopy") {
        generatorResult = Generators::DeepCopy(generatorInfo, templateEngine);
    } else if (gentype == "spv") {
        generatorResult = Generators::Spv(generatorInfo, templateEngine);
    } else {
        std::cerr << "Invalid generator type: " << gentype << ", see help." << std::endl;
        return 1;
    }

    // May have failed
    if (!generatorResult) {
        std::cerr << "Generator failed" << std::endl;
        return 1;
    }

    // Open output
    std::ofstream out(output);
    if (!out.good()) {
        std::cerr << "Failed to open output file: " << output << std::endl;
        return 1;
    }

    // Write template
    out << templateEngine.GetString();

    // OK
    return 0;
}
