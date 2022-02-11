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
