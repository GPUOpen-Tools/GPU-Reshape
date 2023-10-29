#include <Backends/Vulkan/Compiler/Diagnostic/DiagnosticPrettyPrint.h>
#include <Backends/Vulkan/Compiler/Diagnostic/DiagnosticType.h>

// Backend
#include <Backend/Diagnostic/DiagnosticView.h>

void DiagnosticPrettyPrint(const DiagnosticMessage<DiagnosticType> &message, std::stringstream &out) {
    DiagnosticView view(message);

    switch (message.type) {
        default:
            ASSERT(false, "Invalid type");
            break;
        case DiagnosticType::ShaderParsingFailed:
            out << "Shader " << view.Get<uint64_t>() << " - Parsing failed";
            break;
        case DiagnosticType::ShaderInternalCompilerError:
            out << "Shader " << view.Get<uint64_t>() << " - Internal compiler error";
            break;
        case DiagnosticType::PipelineMissingShaderKey:
            out << "Pipeline " << view.Get<uint64_t>() << " - Missing shader stage key";
            break;
        case DiagnosticType::PipelineCreationFailed:
            out << "Pipeline " << view.Get<uint64_t>() << " - Driver creation failed";
            break;
    }
}
