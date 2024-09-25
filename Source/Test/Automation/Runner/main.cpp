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

// Test
#include <Test/Automation/Parser.h>
#include <Test/Automation/TestContainer.h>
#include <Test/Automation/Data/TestData.h>
#include <Test/Automation/Data/HistoryData.h>
#include <Test/Automation/Pass/SequencePass.h>

// Common
#include <Common/Registry.h>
#include <Common/Regex.h>

// Argparse
#include <argparse/argparse.hpp>

// Json
#include <nlohmann/json.hpp>

// Std
#include <iostream>
#include <filesystem>
#include <fstream>

static ComRef<ITestPass> CreateTestContainer(Registry& registry, const std::string& testPath) {
    std::filesystem::path path      = testPath;
    std::filesystem::path directory = path.parent_path();
    
    // All passes
    std::vector<ComRef<ITestPass>> passes;

    // Get regex from filename
    std::regex regex = GetWildcardRegex(path.filename().string());

    // Filter all files
    for (auto&& entry: std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file() || !std::regex_match(entry.path().filename().string(), regex)) {
            continue;
        }
        
        // Try to open json
        std::ifstream stream(entry.path());
        if (!stream.good()) {
            std::cerr << "Failed to open test json file: " << entry.path() << std::endl;
            return nullptr;
        }

        // Parse json
        nlohmann::json testJson{};
        try {
            stream >> testJson;
        } catch(nlohmann::json::exception& ex) {
            std::cerr << "Failed to parse test json file: " << entry.path() << ", " << ex.what() << std::endl;
            return nullptr;
        }
        
        // Try to parse test
        ComRef<ITestPass> testPass;
        if (ComRef parser = registry.New<Parser>(); !parser->Parse(testJson, testPass)) {
            std::cerr << "Failed to parse test: " << entry.path() << std::endl;
            return nullptr;
        }

        // Append
        passes.push_back(testPass);
    }

    // Create as a sequencing pass
    return registry.New<SequencePass>(passes, false);
}

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
    auto &&testFilter = argParser.get<std::string>("-test");
    auto &&filter = argParser.get<std::string>("-filter");

    // Local registry
    Registry registry;

    // Set test data
    ComRef data = registry.AddNew<TestData>();
    data->applicationFilter = filter;

    // Set history data
    ComRef history = registry.AddNew<HistoryData>();
    history->Restore();

    // Create container
    ComRef container = registry.New<TestContainer>();

    // Try to install container
    if (!container->Install()) {
        std::cerr << "Test container failed to install" << std::endl;
        return 1;
    }

    // Create the combined container
    ComRef pass = CreateTestContainer(registry, testFilter);
    if (!pass) {
        return 1;
    }

    // Try to run the tests
    bool result = container->Run(pass);

    // Report result
    std::cout << "\nTest container " << (result ? "passed" : "failed") << std::endl;
    std::cout << "\t" << data->testPassedCount << " passed" << std::endl;
    std::cout << "\t " << data->testFailedCount << " failed" << std::endl;

    // OK (1 is error)
    return !result;
}
