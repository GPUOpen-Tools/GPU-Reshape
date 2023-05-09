#pragma once

// Generator
#include "Schema.h"
#include "Language.h"

// Common
#include <Common/IComponent.h>

// Std
#include <sstream>

/// Schema output stream
struct SchemaStream {
    std::stringstream& header;
    std::stringstream& footer;
};

/// Message output stream
struct MessageStream {
    SchemaStream schema;

    std::stringstream& header;
    std::stringstream& footer;
    std::stringstream& schemaType;
    std::stringstream& chunks;
    std::stringstream& types;
    std::stringstream& functions;
    std::stringstream& members;

    std::string base;
    uint32_t size{0};
};

/// Schema and message generator
class IGenerator : public TComponent<IGenerator> {
public:
    COMPONENT(IGenerator);

    /// Generate the schema
    /// \param schema the schema tree
    /// \param language the target language
    /// \param out the output schema stream
    /// \return success state
    virtual bool Generate(Schema &schema, Language language, SchemaStream& out) = 0;

    /// Generate a message
    /// \param message the message being generated
    /// \param language the target language
    /// \param out the output message stream
    /// \return success state
    virtual bool Generate(const Message& message, Language language, MessageStream& out) = 0;
};
