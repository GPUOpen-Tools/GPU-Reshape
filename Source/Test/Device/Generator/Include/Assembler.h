#pragma once

// Generator
#include "Program.h"

// Common
#include <Common/TemplateEngine.h>

// Std
#include <map>
#include <vector>

/// Assembly info
struct AssembleInfo {
    /// Common paths
    const char* templatePath{nullptr};
    const char* shaderPath{nullptr};

    /// Names
    const char* program{nullptr};
    const char* feature{nullptr};
};

class Assembler {
public:
    Assembler(const AssembleInfo& info, const Program& program);

    /// Assemble to a given stream
    /// \param out the output stream
    /// \return success state
    bool Assemble(std::ostream& out);

private:
    void AssembleConstraints();
    void AssembleResources();

private:
    const Program& program;

    /// Assembly info
    AssembleInfo assembleInfo;

    /// Bucket by message type
    std::map<std::string_view, std::vector<ProgramMessage>> buckets;

    /// Engines
    TemplateEngine testTemplate;
    TemplateEngine constraintTemplate;
};