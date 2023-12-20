// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

#include "GenTypes.h"
#include "Types.h"
#include "Name.h"

// Common
#include <Common/String.h>

// Std
#include <sstream>
#include <regex>

/// Table hader
struct DXILTableHeader {
    std::string name;
    size_t begin;
    size_t end;
};

// Table
struct DXILTable {
    /// Get column value (reserved)
    /// \param rst current rst
    /// \param offset line offset
    /// \param key column identifier
    /// \return empty if not found
    std::string GetColumn(const std::string& rst, size_t offset, const char* key) {
        for (const DXILTableHeader& header : headers) {
            if (header.name == key) {
                return rst.substr(offset + header.begin, header.end - header.begin);
            }
        }

        return "";
    }

    // All local columns
    std::vector<DXILTableHeader> headers;
};

bool Generators::DXILTables(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    std::stringstream enums;

    // Table begin regex
    std::regex tableBegin("\\.\\. ([A-Za-z-]*):BEGIN");

    for(std::sregex_iterator m = std::sregex_iterator(info.dxilRST.begin(), info.dxilRST.end(), tableBegin); m != std::sregex_iterator(); m++) {
        size_t offset = m->position();

        // Skip to table start
        while (info.dxilRST[offset] != '=') {
            offset++;
        }

        // Current table
        DXILTable table;

        // Column pointers
        size_t columnStart = offset;
        size_t columnEnd = info.dxilRST.find('\n', offset);

        // Parse columns
        while (info.dxilRST[offset] != '\n') {
            size_t end = info.dxilRST.find(' ', offset);

            if (end < columnEnd) {
                table.headers.push_back(DXILTableHeader {
                    .name = "",
                    .begin = offset - columnStart,
                    .end = end - columnStart
                });

                offset = end + 1;
            } else {
                table.headers.push_back(DXILTableHeader {
                    .name = "",
                    .begin = offset - columnStart,
                    .end = columnEnd - columnStart
                });

                offset = columnEnd;
            }
        }

        // Eat newline
        offset++;

        // Column pointers
        columnStart = offset;
        columnEnd = info.dxilRST.find('\n', offset);

        // Length of the columns
        size_t columnHeaderLength = columnEnd - columnStart;

        // Get names
        for (DXILTableHeader& header : table.headers) {
            size_t nameOffset = columnStart + header.begin;
            size_t length     = header.end - header.begin;

            // Extract name
            header.name = std::trim_copy(info.dxilRST.substr(nameOffset, std::min(length, columnHeaderLength - header.begin)));
        }

        // Skip line (names)
        while (info.dxilRST[offset++] != '\n');

        // Skip line (table enclosure)
        while (info.dxilRST[offset++] != '\n');

        // Get RST name
        std::string name = (*m)[1].str();

        // Only accept IDs for now
        if (table.headers.empty() || table.headers[0].name != "ID") {
            continue;
        }

        // Export enum
        enums << "enum class DXIL" << GetPrettyName(name.substr(0, name.find("-RST"))) << " {\n";

        // Export values
        while (info.dxilRST[offset] != '=') {
            enums << "\t";
            enums << info.dxilRST.substr(offset + table.headers[1].begin, table.headers[1].end - table.headers[1].begin);
            enums << " = ";
            enums << info.dxilRST.substr(offset + table.headers[0].begin, table.headers[0].end - table.headers[0].begin);
            enums << ",\n";

            // Next
            offset = info.dxilRST.find('\n', offset) + 1;
        }

        // Close
        enums << "};\n\n";
    }

    // Substitute keys
    templateEngine.Substitute("$ENUMS", enums.str().c_str());

    // OK
    return true;
}
