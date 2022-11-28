#pragma once

// Layer
#include "Blocks/SpvPhysicalBlockCapability.h"
#include "Blocks/SpvPhysicalBlockDebugStringSource.h"
#include "Blocks/SpvPhysicalBlockAnnotation.h"
#include "Blocks/SpvPhysicalBlockTypeConstantVariable.h"
#include "Blocks/SpvPhysicalBlockFunction.h"
#include "Utils/SpvUtilShaderExport.h"
#include "Utils/SpvUtilShaderPRMT.h"
#include "Utils/SpvUtilShaderDescriptorConstantData.h"

/// Combined physical block table
struct SpvPhysicalBlockTable {
    SpvPhysicalBlockTable(const Allocators &allocators, Backend::IL::Program &program);

    /// Parse a stream
    /// \param code stream start
    /// \param count stream word count
    /// \return success state
    bool Parse(const uint32_t *code, uint32_t count);

    /// Compile the table
    /// \param job the job to compile against
    /// \return success state
    bool Compile(const SpvJob& job);

    /// Stitch the compiled table
    /// \param out destination stream
    void Stitch(SpvStream &out);

    /// Copy to a new table
    /// \param out the destination table
    void CopyTo(SpvPhysicalBlockTable& out);

    IL::Program& program;

    /// Scanner
    SpvPhysicalBlockScan scan;

    /// Physical blocks
    SpvPhysicalBlockCapability capability;
    SpvPhysicalBlockAnnotation annotation;
    SpvPhysicalBlockDebugStringSource debugStringSource;
    SpvPhysicalBlockTypeConstantVariable typeConstantVariable;
    SpvPhysicalBlockFunction function;

    /// Utilities
    SpvUtilShaderExport shaderExport;
    SpvUtilShaderPRMT shaderPRMT;
    SpvUtilShaderDescriptorConstantData shaderDescriptorConstantData;
};