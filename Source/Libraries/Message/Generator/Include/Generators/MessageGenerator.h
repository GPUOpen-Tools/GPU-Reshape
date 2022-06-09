#pragma once

// Generator
#include <IGenerator.h>
#include "PrimitiveTypeMap.h"

// Std
#include <map>

class MessageGenerator : public IGenerator {
public:
    COMPONENT(MessageGenerator);

    bool Generate(Schema &schema, Language language, SchemaStream& out) override;
    bool Generate(const Message &message, Language language, MessageStream& out) override;

private:
    bool GenerateCPP(const Message &message, MessageStream& out);
    bool GenerateCS(const Message &message, MessageStream& out);

private:
    PrimitiveTypeMap primitiveTypeMap;

    struct TypeMeta {
        size_t size{0};
    };

    // Declared types
    std::map<std::string, TypeMeta> declaredTypes;
};
