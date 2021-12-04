#pragma once

#include <IGenerator.h>
#include <Generators/PrimitiveTypeMap.h>

#include <map>

class ShaderExportGenerator : public IGenerator {
public:
    COMPONENT(ShaderExportGenerator);

    bool Generate(Schema &schema, Language language, const SchemaStream& out) override;
    bool Generate(const Message &message, Language language, const MessageStream& out) override;

private:
    bool GenerateCPP(const Message &message, const MessageStream& out);
    bool GenerateCS(const Message &message, const MessageStream& out);

private:
    PrimitiveTypeMap primitiveTypeMap;

    struct CxxTypeMeta {
        size_t size{0};
    };

    // Declared types
    std::map<std::string, CxxTypeMeta> declaredTypes;
};
