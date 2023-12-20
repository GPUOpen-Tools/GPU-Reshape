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

// Test
#include <Test/Automation/Parser.h>
#include <Test/Automation/TestContainer.h>
#include <Test/Automation/Data/TestData.h>

// Common
#include <Common/Registry.h>

// Argparse
#include <argparse/argparse.hpp>

// Json
#include <nlohmann/json.hpp>

// Std
#include <iostream>
#include <filesystem>
#include <fstream>

int main(int argc, char *const argv[]) {
    argparse::ArgumentParser argParser("GPU Reshape - Automation Runner");

    // Setup parameters
    argParser.add_argument("-test").help("Test json file").default_value(std::string(""));
    argParser.add_argument("-filter").help("Application filter").default_value(std::string(""));

    // Attempt to parse the input
    try {
        argParser.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << argParser;
        return 1;
    }

    // Arguments
    auto &&testPath = argParser.get<std::string>("-test");
    auto &&filter = argParser.get<std::string>("-filter");

    // Try to open json
    std::ifstream stream(testPath);
    if (!stream.good()) {
        std::cerr << "Failed to open test json file: " << testPath << std::endl;
        return 1;
    }

    // Parse json
    nlohmann::json testJson{};
    try {
        stream >> testJson;
    } catch(nlohmann::json::exception& ex) {
        std::cerr << "Failed to parse test json file: " << testPath << ", " << ex.what() << std::endl;
        return 1;
    }

    // Local registry
    Registry registry;

    // Set test data
    ComRef data = registry.AddNew<TestData>();
    data->applicationFilter = filter;

    // Create container
    ComRef container = registry.New<TestContainer>();

    // Try to install container
    if (!container->Install()) {
        std::cerr << "Test container failed to install" << std::endl;
        return 1;
    }

    // Try to parse test
    ComRef<ITestPass> pass;
    if (ComRef parser = registry.New<Parser>(); !parser->Parse(testJson, pass)) {
        std::cerr << "Failed to parse test: " << testPath << std::endl;
        return 1;
    }

    // Try to run the tests
    if (!container->Run(pass)) {
        std::cerr << "Test container failed: " << testPath << std::endl;
        return 1;
    }

    // OK
    return 0;
}
