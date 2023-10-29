#include <Backends/DX12/Compiler/Diagnostic/DiagnosticPrettyPrint.h>
#include <Backends/DX12/Compiler/Diagnostic/DiagnosticType.h>

// Backend
#include <Backend/Diagnostic/DiagnosticView.h>

void DiagnosticPrettyPrint(const DiagnosticMessage<DiagnosticType> &message, std::stringstream &out) {
    DiagnosticView view(message);

    switch (message.type) {
        default:
            ASSERT(false, "Invalid type");
            break;
        case DiagnosticType::ShaderUnknownHeader:
            out << "Shader " << view.Get<uint64_t>() << " - Unknown bytecode header " << view.Get<uint32_t>();
            break;
        case DiagnosticType::ShaderInternalCompilerError:
            out << "Shader " << view.Get<uint64_t>() << " - Internal compiler error";
            break;
        case DiagnosticType::ShaderNativeDXBCNotSupported:
            out << "Shader " << view.Get<uint64_t>() << " - Native DXBC not supported, enable experimental conversion";
            break;
        case DiagnosticType::ShaderDXILSigningFailed:
            out << "Shader " << view.Get<uint64_t>() << " - Failed to sign DXIL bytecode";
            break;
        case DiagnosticType::ShaderDXBCSigningFailed:
            out << "Shader " << view.Get<uint64_t>() << " - Failed to sign DXBC bytecode";
            break;
        case DiagnosticType::PipelineMissingShaderKey:
            out << "Pipeline " << view.Get<uint64_t>() << " - Missing shader stage key";
            break;
        case DiagnosticType::PipelineCreationFailed:
            out << "Pipeline " << view.Get<uint64_t>() << " - Driver creation failed";
            break;
        case DiagnosticType::PipelineInternalError:
            out << "Pipeline " << view.Get<uint64_t>() << " - Internal error";
            break;
    }
}
