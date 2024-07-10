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

// Backend
#include <Backend/IL/PrettyGraph.h>
#include <Backend/IL/PrettyPrint.h>

// Common
#include <Common/FileSystem.h>

// System
#ifdef WIN32
#   include <Windows.h>
#else // WIN32
#   error Not implemented
#endif // WIN32

// Std
#include <fstream>

void IL::PrettyDotGraph(const Function &function, const std::filesystem::path& dotOutput, const std::filesystem::path& pngOutput) {
    // Pretty print the graph
    std::ofstream out(dotOutput);
    IL::PrettyPrintBlockDotGraph(function, IL::PrettyPrintContext(out));
    out.close();

    // Toolset path
    std::filesystem::path graphVizDir = GetBaseModuleDirectory() / "GraphViz";

    // Check if the toolset is available
    if (!std::filesystem::exists(graphVizDir)) {
        return;
    }
    
#ifdef _MSC_VER
    // Startup information
    STARTUPINFO startupInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));

    // Populate
    startupInfo.cb = sizeof(startupInfo);

    // Process information
    PROCESS_INFORMATION processInfo;
    ZeroMemory(&processInfo, sizeof(processInfo));

    // Output
    std::stringstream ss;
    ss << " -Tpng";
    ss << " -o" << pngOutput;
    ss << " " << dotOutput;

    // Run graph viz
    if (!CreateProcess(
        (graphVizDir / "dot.exe").string().c_str(),
        ss.str().data(),
        nullptr,
        nullptr,
        false,
        0,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo
    )) {
        return;
    }

    // Wait for process
    WaitForSingleObject(processInfo.hProcess, INFINITE);

    // Cleanup
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
#else // WIN32
#   error Not implemented
#endif // _MSC_VER
}
