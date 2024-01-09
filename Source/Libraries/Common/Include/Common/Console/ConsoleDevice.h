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

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <sstream>

class ConsoleDevice {
public:
    /// Parse the next command
    /// \return false if exit signal
    bool Next() {
        // Cleanup
        segments.clear();

        // Command syntax
        std::cout << ">> " << std::flush;

        // Get line
        std::string line;
        std::getline(std::cin, line);

        // Parse command
        std::istringstream iss(line);
        std::copy(std::istream_iterator<std::string>(iss),
                  std::istream_iterator<std::string>(),
                  std::back_inserter(segments));

        if (Is("stop")) {
            return false;
        }

        return true;
    }

    std::string Command() const {
        if (segments.empty()) {
            return "";
        }
        return segments.at(0);
    }

    bool Is(const std::string& command) const {
        return Command() == command;
    }

    uint32_t ArgCount() {
        return segments.empty() ? 0 : static_cast<uint32_t>(segments.size()) - 1;
    }

    std::string Arg(uint32_t n) const {
        if (n + 1 >= segments.size()) {
            return "";
        }
        return segments.at(n + 1);
    }

private:
    std::vector<std::string> segments;
};
