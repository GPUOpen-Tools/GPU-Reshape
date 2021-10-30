// Std
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

// Argparse
#include <argparse/argparse.hpp>

// Xml
#include <tinyxml2.h>

// Common
#include <Common/TemplateEngine.h>

// Generator
#include "GenTypes.h"

int main(int argc, char *const argv[]) {
    argparse::ArgumentParser program("GPUOpen GBV - Message Generator");

    // Setup parameters
    program.add_argument("-schemaxml").help("Path of the schema xml file").required();
    program.add_argument("-template").help("The file to template").required();
    program.add_argument("-lang").help("Languages to generator for, limited to [cpp, cs]").default_value(std::string(""));
    program.add_argument("-opath").help("Output directory of the generated files").required();

    // Attempt to parse the input
    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    // Arguments
    auto &&schemaxml = program.get<std::string>("-schemaxml");
    auto &&outputpath = program.get<std::string>("-opath");
    auto &&ftemplate = program.get<std::string>("-template");
    auto &&langs = program.get<std::string>("-lang");

    // Generation info
    GeneratorInfo info;

    // Parse languages
    std::stringstream langStream(langs);
    while( langStream.good() ) {
        std::string substr;
        getline(langStream, substr, ',' );
        info.languages.insert(substr);
    }

    // Attempt to open the specification xml
    tinyxml2::XMLDocument document;
    if (document.LoadFile(schemaxml.c_str()) != tinyxml2::XML_SUCCESS) {
        std::cerr << "Failed to open schemaxml file: " << schemaxml << std::endl;
        return 1;
    }

    // Get the root registry
    info.schema = document.FirstChildElement("schema");
    if (!info.schema) {
        std::cerr << "Failed to find root schema in xml" << std::endl;
        return 1;
    }

    // Language to template mappings
    std::map<std::string, std::string> languageInMappings = {
        { "cpp", ".ht" },
        { "cs", ".cst" }
    };

    // Language to template mappings
    std::map<std::string, std::string> languageOutMappings = {
        { "cpp", ".h" },
        { "cs", ".cs" }
    };

    // Process languages
    for (const std::string& lang : info.languages) {
        // Try to open the template
        TemplateEngine templateEngine;
        if (!templateEngine.Load((ftemplate + languageInMappings[lang]).c_str())) {
            std::cerr << "Failed to open template file: " << ftemplate << std::endl;
            return 1;
        }

        // Generator result
        bool result{};

        // Invoke generator
        if (lang == "cpp") {
            result = Generators::CPP(info, templateEngine);
        } else if (lang == "cs") {
            result = Generators::CS(info, templateEngine);
        } else {
            std::cerr << "Invalid language type: " << lang << ", see help." << std::endl;
            return 1;
        }

        // May have failed
        if (!result) {
            std::cerr << "Generator failed" << std::endl;
            return 1;
        }

        // Output path
        std::filesystem::path outputFilename = std::filesystem::path(schemaxml).stem().string() + languageOutMappings[lang];
        std::filesystem::path outputPath = std::filesystem::path(outputpath) / outputFilename;

        // Open output
        std::ofstream out(outputPath.c_str());
        if (!out.good()) {
            std::cerr << "Failed to open output file: " << outputPath << std::endl;
            return 1;
        }

        // Write template
        out << templateEngine.GetString();
    }

    // OK
    return 0;
}
