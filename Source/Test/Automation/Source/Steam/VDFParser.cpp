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

#include <Test/Automation/Steam/VDFParser.h>
#include <Test/Automation/Steam/VDFParseContext.h>

VDFParser::VDFParser(VDFArena &arena): arena(arena) {
    
}

bool VDFParser::Parse(const char *contents, VDFNode **out) {
    VDFParseContext ctx(contents);

    // Create node
    auto dict = arena.Allocate<VDFDictionaryNode>();
    dict->type = VDFNodeType::Dictionary;
    dict->entryCount = 1;
    dict->entries = arena.Allocate<VDFDictionaryEntry>();

    // Parse root entry
    if (!ParseDictionaryEntry(ctx, dict->entries[0])) {
        return false;
    }

    // OK
    *out = dict;
    return true;
}

bool VDFParser::ParseNode(VDFParseContext &ctx, VDFNode **out) {
    // Handle dicts separately
    if (ctx.Is('{')) {
        return ParseDictionary(ctx, out);
    }

    // Assume string
    auto node = arena.Allocate<VDFString>();
    node->type = VDFNodeType::String;

    // Try to parse as string
    if (!ParseString(ctx, node->string)) {
        return false;
    }

    // OK
    *out = node;
    return true;
}

bool VDFParser::ParseDictionary(VDFParseContext &ctx, VDFNode **out) {
    ctx.Skip();

    // All entries
    std::vector<VDFDictionaryEntry> entries;

    // Parse entries until end
    while (ctx.Good() && !ctx.Is('}')) {
        if (!ParseDictionaryEntry(ctx, entries.emplace_back())) {
            return false;
        }
    }

    // EOF before closure?
    if (!ctx.IsConsume('}')) {
        return false;
    }

    // Create dict
    auto dict = arena.Allocate<VDFDictionaryNode>();
    dict->type = VDFNodeType::Dictionary;
    dict->entryCount = static_cast<uint32_t>(entries.size());
    dict->entries = arena.AllocateArray<VDFDictionaryEntry>(dict->entryCount);
    std::memcpy(dict->entries, entries.data(), sizeof(VDFDictionaryEntry) * dict->entryCount);

    // OK
    *out = dict;
    return true;
}

bool VDFParser::ParseDictionaryEntry(VDFParseContext &ctx, VDFDictionaryEntry &out) {
    // Try to parse key
    if (!ParseString(ctx, out.key)) {
        return false;
    }

    // Try to parse value
    if (!ParseNode(ctx, &out.node)) {
        return false;
    }

    // OK
    return true;
}

bool VDFParser::ParseString(VDFParseContext &ctx, std::string_view &out) {
    if (!ctx.IsConsume('"')) {
        return false;
    }

    // VDF allows for escape characters, replace double slashes
    std::string str = replace_all(ctx.ConsumeWith('"'), "\\\\", "\\");

    // Copy owernship to arena
    auto* data = arena.AllocateArray<char>(static_cast<uint32_t>(str.length() + 1));
    std::memcpy(data, str.data(), str.length());
    data[str.length()] = '\0';

    // OK
    out = std::string_view(data, str.length());
    return true;
}
