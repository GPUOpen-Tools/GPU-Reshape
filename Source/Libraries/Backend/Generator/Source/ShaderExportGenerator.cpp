#include <ShaderExportGenerator.h>

#include <Backend/ShaderExport.h>
#include <sstream>
#include <iostream>

bool ShaderExportGenerator::Generate(Schema &schema, Language language, const SchemaStream &out) {
    for (Message& message : schema.messages) {
        // Append shader guid if not disabled
        if (!message.attributes.GetBool("no-sguid")) {
            Field& sguid = *message.fields.emplace(message.fields.begin());
            sguid.name = "sguid";
            sguid.type = "uint16";

            // Attributes
            sguid.attributes.Add("bits", std::to_string(kShaderSGUIDBitCount));
        }
    }

    // Include emitter
    out.header << "#include <Backend/IL/Emitter.h>\n";

    // OK
    return true;
}

bool ShaderExportGenerator::Generate(const Message &message, Language language, const MessageStream &out) {
    switch (language) {
        case Language::CPP:
            return GenerateCPP(message, out);
        case Language::CS:
            return GenerateCS(message, out);
    }
}

bool ShaderExportGenerator::GenerateCPP(const Message &message, const MessageStream &out) {
    // Begin shader type
    out.types << "\tstruct ShaderExport {\n";

    // SGUID?
    bool noSGUID = message.attributes.GetBool("no-sguid");
    out.types << "\t\tstatic constexpr bool kNoSGUID = " << (noSGUID ? "true" : "false") << ";\n";

    // Structured?
    bool structured = message.attributes.GetBool("structured");
    out.types << "\t\tstatic constexpr bool kStructured = " << (structured ? "true" : "false") << ";\n\n";

    // Begin construction function
    out.types << "\t\ttemplate<typename OP>\n";
    out.types << "\t\tIL::ID Construct(IL::Emitter<OP>& emitter) const {\n";

    // Simple write?
    if (!structured) {
        // Allocate type
        out.types << "\t\t\tIL::ID value = emitter.UInt(32, 0);\n";

        // Current offset
        uint32_t bitOffset = 0;

        // Append all fields
        for (const Field& field : message.fields) {
            auto it = primitiveTypeMap.types.find(field.type);
            if (it == primitiveTypeMap.types.end()) {
                std::cerr << "Malformed command in line: " << message.line << ", type " << field.type << " not supported for non structured writes" << std::endl;
                return false;
            }

            // Optional bit size
            auto bits = field.attributes.Get("bits");

            // Determine the size of this field
            uint32_t bitSize = bits ? std::atoi(bits->value.c_str()) : (it->second.size * 8);

            // Append value
            out.types << "\t\t\tvalue = emitter.BitOr(value, emitter.BitShiftLeft(" << field.name << ", emitter.UInt(32, " << bitOffset << ")));\n";

            // Next
            bitOffset += bitSize;
        }

        // Check non structured write limit
        if (bitOffset > 32) {
            std::cerr << "Malformed command in line: " << message.line << ", non structured size exceeded 32 bits with " << bitOffset << " bits" << std::endl;
            return false;
        }

        // Done!
        out.types << "\t\t\treturn value;\n";
    } else {
        // Soon (tm)
        std::cerr << "Malformed command in line: " << message.line << ", structured writes not supported yet" << std::endl;
        return false;
    }

    // End construction function
    out.types << "\t\t}\n\n";

    // Begin shader values
    for (const Field& field : message.fields) {
        out.types << "\t\tIL::ID " << field.name << "{IL::InvalidID};\n";
    }

    // End shader type
    out.types << "\t};\n\n";
    return true;
}

bool ShaderExportGenerator::GenerateCS(const Message &message, const MessageStream &out) {
    return true;
}
