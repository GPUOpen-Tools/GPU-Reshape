// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
#include <filesystem>

// Argparse
#include <argparse/argparse.hpp>

// Xml
#include <tinyxml2.h>

// Common
#include <Common/TemplateEngine.h>
#include <Common/Registry.h>
#include <Common/IDHash.h>
#include <Common/Library.h>

// Generator
#include <GeneratorHost.h>
#include <Generators/MessageGenerator.h>
#include <Plugin.h>
#include <Schema.h>
#include <Language.h>

int main(int argc, char *const argv[]) {
    argparse::ArgumentParser program("GPUOpen GBV - Message Generator");

    // Setup parameters
    program.add_argument("-libs").help("Path of a library for extensions").default_value(std::string(""));
    program.add_argument("-templates").help("Path of the templates").required();
    program.add_argument("-schemaxml").help("Path of the schema xml file").required();
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
    auto &&libs = program.get<std::string>("-libs");
    auto &&templates = program.get<std::string>("-templates");
    auto &&schemaxml = program.get<std::string>("-schemaxml");
    auto &&outputpath = program.get<std::string>("-opath");
    auto &&langs = program.get<std::string>("-lang");

    // Libraries to import
    std::vector<Library> libraries;

    // Parse libraries
    std::stringstream libStream(libs);
    while( libStream.good() && !libs.empty() ) {
        std::string libraryPath;
        getline(libStream, libraryPath, ',');

        // Try to load the library
        Library& library = libraries.emplace_back();
        if (!library.Load(libraryPath.c_str())) {
            std::cerr << "Failed to load library: '" << libraryPath << "'" << std::endl;
            return 1;
        }
    }

    // Shared registry
    Registry registry;

    // Install the message generator
    auto messageGenerator = registry.AddNew<MessageGenerator>();

    // Setup the host
    GeneratorHost host;
    host.AddMessage("struct", messageGenerator);
    host.AddMessage("message", messageGenerator);

    // Install all libraries
    for (Library& lib : libraries) {
        auto* install = lib.GetFn<PluginInstall>("Install");
        if (!install) {
            std::cerr << "Library has no install proc: '" << lib.Path() << "'" << std::endl;
            return 1;
        }

        // Try to install
        if (!install(&registry, &host)) {
            std::cerr << "Failed to install library: '" << lib.Path() << "'" << std::endl;
            return 1;
        }
    }

    // Languages to export
    std::vector<Language> languages;

    // Parse languages
    std::stringstream langStream(langs);
    while( langStream.good() ) {
        std::string lang;
        getline(langStream, lang, ',' );

        // Supported languages
        static std::map<std::string, Language> map = {
            {"cpp", Language::CPP},
            {"cs", Language::CS},
        };

        // Check if supported
        auto it = map.find(lang);
        if (it == map.end()) {
            std::cerr << "Invalid language type: " << lang << ", see help." << std::endl;
            return 1;
        }

        languages.push_back(it->second);
    }

    // Attempt to open the specification xml
    tinyxml2::XMLDocument document;
    if (document.LoadFile(schemaxml.c_str()) != tinyxml2::XML_SUCCESS) {
        std::cerr << "Failed to open schemaxml file: " << schemaxml << std::endl;
        return 1;
    }

    // Processed schema tree
    Schema schema;

    // Get the root registry
    tinyxml2::XMLElement* schemaNode = document.FirstChildElement("schema");
    if (!schemaNode) {
        std::cerr << "Failed to find root schema in xml" << std::endl;
        return 1;
    }

    // Process all nodes
    for (tinyxml2::XMLNode *commandNode = schemaNode->FirstChild(); commandNode; commandNode = commandNode->NextSibling()) {
        tinyxml2::XMLElement *command = commandNode->ToElement();

        // Processed message
        Message& message = schema.messages.emplace_back();
        message.line = command->GetLineNum();

        // Get the type
        message.type = command->Name();

        // Get the name
        const char *name = command->Attribute("name", nullptr);
        if (!name) {
            std::cerr << "Malformed command in line: " << command->GetLineNum() << ", name not found" << std::endl;
            return 1;
        }

        // Set name
        message.name = name;

        // Get all attributes
        for (const tinyxml2::XMLAttribute* attributeNode = command->FirstAttribute(); attributeNode; attributeNode = attributeNode->Next()) {
            Attribute& attr = message.attributes.attributes.emplace_back();
            attr.name = attributeNode->Name();
            attr.value = attributeNode->Value();
        }

        // Iterate all parameters
        for (tinyxml2::XMLNode *childNode = commandNode->FirstChild(); childNode; childNode = childNode->NextSibling()) {
            tinyxml2::XMLElement *element = childNode->ToElement();
            if (!element) {
                std::cerr << "Malformed child in line: " << childNode->GetLineNum() << std::endl;
                return 1;
            }

            // Field type?
            if (!std::strcmp(element->Name(), "field")) {
                Field &field = message.fields.emplace_back();

                // Get the name
                const char *fieldName = element->Attribute("name", nullptr);
                if (!fieldName) {
                    std::cerr << "Malformed command in line: " << element->GetLineNum() << ", name not found" << std::endl;
                    return 1;
                }

                // Get the type
                const char *typeName = element->Attribute("type", nullptr);
                if (!typeName) {
                    std::cerr << "Malformed command in line: " << element->GetLineNum() << ", type not found" << std::endl;
                    return 1;
                }

                // Set basic properties
                field.name = fieldName;
                field.type = typeName;
                field.line = element->GetLineNum();

                // Get all attributes
                for (const tinyxml2::XMLAttribute *attributeNode = element->FirstAttribute(); attributeNode; attributeNode = attributeNode->Next()) {
                    Attribute &attr = field.attributes.attributes.emplace_back();
                    attr.name = attributeNode->Name();
                    attr.value = attributeNode->Value();
                }
            } else if (!std::strcmp(element->Name(), "chunk")) {
                Chunk &chunk = message.chunks.emplace_back();

                // Get the name
                const char *chunkName = element->Attribute("name", nullptr);
                if (!chunkName) {
                    std::cerr << "Malformed command in line: " << element->GetLineNum() << ", name not found" << std::endl;
                    return 1;
                }

                // Set basic properties
                chunk.name = chunkName;
                chunk.line = element->GetLineNum();

                // Get all attributes
                for (const tinyxml2::XMLAttribute *attributeNode = element->FirstAttribute(); attributeNode; attributeNode = attributeNode->Next()) {
                    Attribute &attr = chunk.attributes.attributes.emplace_back();
                    attr.name = attributeNode->Name();
                    attr.value = attributeNode->Value();
                }

                // Iterate all parameters
                for (tinyxml2::XMLNode *chunkChildNode = element->FirstChild(); chunkChildNode; chunkChildNode = chunkChildNode->NextSibling()) {
                    tinyxml2::XMLElement *chunkElement = chunkChildNode->ToElement();
                    if (!chunkElement) {
                        std::cerr << "Malformed child in line: " << chunkChildNode->GetLineNum() << std::endl;
                        return 1;
                    }

                    // Field type?
                    if (!std::strcmp(chunkElement->Name(), "field")) {
                        Field &field = chunk.fields.emplace_back();

                        // Get the name
                        const char *fieldName = chunkElement->Attribute("name", nullptr);
                        if (!fieldName) {
                            std::cerr << "Malformed command in line: " << chunkElement->GetLineNum() << ", name not found" << std::endl;
                            return 1;
                        }

                        // Get the type
                        const char *typeName = chunkElement->Attribute("type", nullptr);
                        if (!typeName) {
                            std::cerr << "Malformed command in line: " << chunkElement->GetLineNum() << ", type not found" << std::endl;
                            return 1;
                        }

                        // Set basic properties
                        field.name = fieldName;
                        field.type = typeName;
                        field.line = chunkElement->GetLineNum();

                        // Get all attributes
                        for (const tinyxml2::XMLAttribute *attributeNode = chunkElement->FirstAttribute(); attributeNode; attributeNode = attributeNode->Next()) {
                            Attribute &attr = field.attributes.attributes.emplace_back();
                            attr.name = attributeNode->Name();
                            attr.value = attributeNode->Value();
                        }
                    } else {
                        std::cerr << "Malformed child in line: " << chunkChildNode->GetLineNum() << ", unknown child type: '" << chunkElement->Name() << "'" << std::endl;
                        return 1;
                    }
                }
            } else {
                std::cerr << "Malformed child in line: " << childNode->GetLineNum() << ", unknown child type: '" << element->Name() << "'" << std::endl;
                return 1;
            }
        }
    }

    // Language to template mappings
    std::map<Language, std::string> languageInMappings = {
        { Language::CPP, ".ht" },
        { Language::CS, ".cst" }
    };

    // Language to template mappings
    std::map<Language, std::string> languageOutMappings = {
        { Language::CPP, ".h" },
        { Language::CS, ".cs" }
    };

    // Process languages
    for (Language lang : languages) {
        // Try to open the template
        TemplateEngine schemaTemplate;
        if (!schemaTemplate.Load((templates + "/Schema" + languageInMappings[lang]).c_str())) {
            std::cerr << "Failed to open schema template" << std::endl;
            return 1;
        }

        // Streams
        std::stringstream headerStream;
        std::stringstream messageStream;
        std::stringstream footerStream;

        // Schema output
        SchemaStream schemaStreamOut{
            headerStream,
            footerStream
        };

        // Invoke generator on schema
        for (const ComRef<IGenerator>& gen : host.GetSchemaGenerators()) {
            if (!gen->Generate(schema, lang, schemaStreamOut)) {
                std::cerr << "Schema generator failed" << std::endl;
                return 1;
            }
        }

        // Try to open the template
        TemplateEngine messageTemplate;
        if (!messageTemplate.Load((templates + "/Message" + languageInMappings[lang]).c_str())) {
            std::cerr << "Failed to open message template" << std::endl;
            return 1;
        }

        // Generate all messages
        for (const Message& message : schema.messages) {
            // Streams
            std::stringstream header;
            std::stringstream footer;
            std::stringstream chunksStream;
            std::stringstream schemaType;
            std::stringstream typeStream;
            std::stringstream functionStream;
            std::stringstream memberStream;

            // Schema output
            MessageStream messageStreamOut{
                schemaStreamOut,
                header,
                footer,
                schemaType,
                chunksStream,
                typeStream,
                functionStream,
                memberStream
            };

            // Invoke generator on message
            for (const ComRef<IGenerator>& gen : host.GetMessageGenerators(message.type)) {
                if (!gen->Generate(message, lang, messageStreamOut)) {
                    std::cerr << "Message generator failed" << std::endl;
                    return 1;
                }
            }

            // Message name
            std::string name = message.name + "Message";

            // Get the hash
            std::string hash = std::to_string(IDHash(message.name.c_str())) + "u";

            // Instantiate optionals
            messageTemplate.Substitute("$SIZE", std::to_string(messageStreamOut.size).c_str());
            messageTemplate.Substitute("$SCHEMA", schemaType.str().c_str());
            messageTemplate.Substitute("$BASE", messageStreamOut.base.c_str());

            // Instantiate message
            if (!messageTemplate.Substitute("$NAME", name.c_str()) ||
                !messageTemplate.Substitute("$ID", hash.c_str()) ||
                !messageTemplate.Substitute("$HEADER", header.str().c_str()) ||
                !messageTemplate.Substitute("$FOOTER", footer.str().c_str()) ||
                !messageTemplate.Substitute("$CHUNKS", chunksStream.str().c_str()) ||
                !messageTemplate.Substitute("$TYPES", typeStream.str().c_str()) ||
                !messageTemplate.Substitute("$FUNCTIONS", functionStream.str().c_str()) ||
                !messageTemplate.Substitute("$MEMBERS", memberStream.str().c_str())) {
                std::cerr << "Bad message template, failed to substitute" << std::endl;
                return 1;
            }

            // Append
            messageStream << messageTemplate.GetString();
            messageTemplate.Reset();
        }

        // Instantiate schema
        if (!schemaTemplate.Substitute("$HEADER", headerStream.str().c_str()) ||
            !schemaTemplate.Substitute("$MESSAGES", messageStream.str().c_str()) ||
            !schemaTemplate.Substitute("$FOOTER", footerStream.str().c_str())) {
            std::cerr << "Bad schema template, failed to substitute" << std::endl;
            return 1;
        }

        // Output path
        std::string outputFilename = std::filesystem::path(schemaxml).stem().string() + languageOutMappings[lang];
        std::string outputPath = outputpath  + "/" + outputFilename;

        // Open output
        std::ofstream out(outputPath.c_str());
        if (!out.good()) {
            std::cerr << "Failed to open output file: " << outputPath << std::endl;
            return 1;
        }

        // Write template
        out << schemaTemplate.GetString();
    }

    // Uninstall all libraries
    for (Library& lib : libraries) {
        auto* uninstall = lib.GetFn<PluginInstall>("Uninstall");
        if (!uninstall) {
            std::cerr << "Library has no uninstall proc: '" << lib.Path() << "'" << std::endl;
            return 1;
        }

        uninstall(&registry, &host);
    }

    // OK
    return 0;
}
