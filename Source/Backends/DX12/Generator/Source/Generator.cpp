// Std
#include <iostream>
#include <sstream>
#include <fstream>

// Argparse
#include <argparse/argparse.hpp>

// Common
#include <Common/TemplateEngine.h>

// Generator
#include "GenTypes.h"

int main(int argc, char *const argv[]) {
    argparse::ArgumentParser program("GPUOpen GBV - DX12 Generator");

    // Setup parameters
    program.add_argument("-specjson").help("Path of the specification json file").default_value(std::string(""));
    program.add_argument("-dxilrst").help("Path of the dxil rst file").default_value(std::string(""));
    program.add_argument("-hooksjson").help("Path of the hooks json file").default_value(std::string(""));
    program.add_argument("-deepcopyjson").help("Path of the deep copy json file").default_value(std::string(""));
    program.add_argument("-template").help("The file to template").required();
    program.add_argument("-gentype").help("The generation type, one of [specification, detour, wrappers, wrappersimpl, vtable, table, deepcopy, deepcopyimpl, dxil]").required();
    program.add_argument("-d3d12h").help("The d3d12 header file").default_value(std::string(""));
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
    auto &&specjson = program.get<std::string>("-specjson");
    auto &&dxilrst = program.get<std::string>("-dxilrst");
    auto &&hooksjson = program.get<std::string>("-hooksjson");
    auto &&deepcopyjson = program.get<std::string>("-deepcopyjson");
    auto &&ftemplate = program.get<std::string>("-template");
    auto &&gentype = program.get<std::string>("-gentype");
    auto &&d3d12h = program.get<std::string>("-d3d12h");
    auto &&output = program.get<std::string>("-o");

    // Generator information
    GeneratorInfo generatorInfo{};
    generatorInfo.d3d12HeaderPath = d3d12h;

    // Parse optional json
    if (!specjson.empty()) {
        // Open json
        std::ifstream stream(specjson);
        if (!stream.good()) {
            std::cerr << "Failed to open json file: " << specjson << std::endl;
            return 1;
        }

        // Parse json
        try {
            stream >> generatorInfo.specification;
        } catch(nlohmann::json::exception& ex) {
            std::cerr << "Failed to parse json file: " << specjson << ", " << ex.what() << std::endl;
            return 1;
        }
    }

    // Parse optional dxil rst
    if (!dxilrst.empty()) {
        std::ifstream file(dxilrst);

        // Store
        std::stringstream ss;
        ss << file.rdbuf();
        generatorInfo.dxilRST = ss.str();
    }

    // Parse optional hooks json
    if (!hooksjson.empty()) {
        // Open json
        std::ifstream stream(hooksjson);
        if (!stream.good()) {
            std::cerr << "Failed to open json file: " << hooksjson << std::endl;
            return 1;
        }

        // Parse json
        try {
            stream >> generatorInfo.hooks;
        } catch(nlohmann::json::exception& ex) {
            std::cerr << "Failed to parse json file: " << hooksjson << ", " << ex.what() << std::endl;
            return 1;
        }
    }

    // Parse optional deep copy json
    if (!deepcopyjson.empty()) {
        // Open json
        std::ifstream stream(deepcopyjson);
        if (!stream.good()) {
            std::cerr << "Failed to open json file: " << deepcopyjson << std::endl;
            return 1;
        }

        // Parse json
        try {
            stream >> generatorInfo.deepCopy;
        } catch(nlohmann::json::exception& ex) {
            std::cerr << "Failed to parse json file: " << deepcopyjson << ", " << ex.what() << std::endl;
            return 1;
        }
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
    if (gentype == "specification") {
        generatorResult = Generators::Specification(generatorInfo, templateEngine);
    } else if (gentype == "detour") {
        generatorResult = Generators::Detour(generatorInfo, templateEngine);
    } else if (gentype == "wrappers") {
        generatorResult = Generators::Wrappers(generatorInfo, templateEngine);
    } else if (gentype == "wrappersimpl") {
        generatorResult = Generators::WrappersImpl(generatorInfo, templateEngine);
    } else if (gentype == "vtable") {
        generatorResult = Generators::VTable(generatorInfo, templateEngine);
    } else if (gentype == "table") {
        generatorResult = Generators::Table(generatorInfo, templateEngine);
    } else if (gentype == "deepcopy") {
        generatorResult = Generators::DeepCopy(generatorInfo, templateEngine);
    } else if (gentype == "deepcopyimpl") {
        generatorResult = Generators::DeepCopyImpl(generatorInfo, templateEngine);
    } else if (gentype == "dxil") {
        generatorResult = Generators::DXIL(generatorInfo, templateEngine);
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
