// Std
#include <iostream>
#include <sstream>
#include <fstream>

// Argparse
#include <argparse/argparse.hpp>

// Xml
#include <tinyxml2.h>

// Generator
#include "TemplateEngine.h"
#include "GenTypes.h"

int main(int argc, char *const argv[]) {
    argparse::ArgumentParser program("GPUOpen GBV - Vulkan Generator");

    // Setup parameters
    program.add_argument("-vkxml").help("Path of the vulkan specification xml file").required();
    program.add_argument("-template").help("The file to template").required();
    program.add_argument("-gentype").help("The generation type, one of [commandbuffer, commandbufferdispatchtable]").required();
    program.add_argument("-whitelist").help("Whitelist a hook").default_value(std::string(""));
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
    auto &&output = program.get<std::string>("-o");
    auto &&whitelist = program.get<std::string>("-whitelist");

    // Generator information
    GeneratorInfo generatorInfo{};

    // Parse whitelist
    std::stringstream whitelistStream(whitelist);
    while( whitelistStream.good() ) {
        std::string substr;
        getline(whitelistStream, substr, ',' );
        generatorInfo.Whitelist.insert( substr );
    }

    // Attempt to open the specification xml
    tinyxml2::XMLDocument document;
    if (document.LoadFile(vkxml.c_str()) != tinyxml2::XML_SUCCESS) {
        std::cerr << "Failed to open vkxml file: " << vkxml << std::endl;
        return 1;
    }

    // Get the root registry
    generatorInfo.Registry = document.FirstChildElement("registry");
    if (!generatorInfo.Registry) {
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
