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

bool Generators::DXIL(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    std::stringstream enums;

    // Current search string
    const char* searchStr = info.dxilRST.c_str();

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

        // Next!
        searchStr += m->position() + m->length();
    }

    // Substitute keys
    templateEngine.Substitute("$ENUMS", enums.str().c_str());

    // OK
    return true;
}
