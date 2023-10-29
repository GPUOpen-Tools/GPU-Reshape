#pragma once

enum class DiagnosticType {
    None,
    
    /** Shader diagnostics */
    ShaderUnknownHeader,
    ShaderInternalCompilerError,
    ShaderNativeDXBCNotSupported,
    ShaderDXILSigningFailed,
    ShaderDXBCSigningFailed,

    /** Pipeline diagnostics */
    PipelineMissingShaderKey,
    PipelineCreationFailed,
    PipelineInternalError,
};
