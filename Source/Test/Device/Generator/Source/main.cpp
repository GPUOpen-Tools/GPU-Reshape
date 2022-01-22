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
    argparse::ArgumentParser argParser("GPUOpen GBV - TestSuite Backend generator");

    // Setup parameters
    argParser.add_argument("-test").help("Test file").required();
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
    auto &&test = argParser.get<std::string>("-test");
    auto &&templates = argParser.get<std::string>("-templates");
    auto &&shader = argParser.get<std::string>("-shader");
    auto &&feature = argParser.get<std::string>("-feature");
    auto &&out = argParser.get<std::string>("-o");

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

    // OK
    return 0;
}