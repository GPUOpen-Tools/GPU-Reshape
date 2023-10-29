#pragma once

enum class DiagnosticType {
    None,
    
    /** Shader diagnostics */
    ShaderParsingFailed,
    ShaderInternalCompilerError,

    /** Pipeline diagnostics */
    PipelineMissingShaderKey,
    PipelineCreationFailed
};
