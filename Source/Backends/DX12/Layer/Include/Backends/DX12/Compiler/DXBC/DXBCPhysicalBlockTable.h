#pragma once

// Layer
#include "DXBCPhysicalBlockScan.h"
#include "Blocks/DXBCPhysicalBlockShader.h"
#include "Blocks/DXBCPhysicalBlockPipelineStateValidation.h"
#include "Blocks/DXBCPhysicalBlockRootSignature.h"
#include "Blocks/DXBCPhysicalBlockFeatureInfo.h"

// Forward declarations
struct DXJob;
struct DXStream;
struct DXILModule;
struct IDXDebugModule;

/// Complete physical block table
struct DXBCPhysicalBlockTable {
    DXBCPhysicalBlockTable(const Allocators &allocators, IL::Program &program);

    /// Scan the DXBC bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    bool Parse(const void* byteCode, uint64_t byteLength);

    /// Compile the table
    /// \param job the job to compile against
    /// \return success state
    bool Compile(const DXJob& job);

    /// Stitch the compiled table
    /// \param out destination stream
    void Stitch(const DXJob& job, DXStream &out);

    /// Copy to a new table
    /// \param out the destination table
    void CopyTo(DXBCPhysicalBlockTable& out);

    /// Scanner
    DXBCPhysicalBlockScan scan;

    /// Blocks
    DXBCPhysicalBlockShader shader;
    DXBCPhysicalBlockPipelineStateValidation pipelineStateValidation;
    DXBCPhysicalBlockRootSignature rootSignature;
    DXBCPhysicalBlockFeatureInfo featureInfo;

    /// DXBC containers can host DXIL data
    DXILModule* dxilModule{nullptr};

    /// Optional debug data
    IDXDebugModule* debugModule{nullptr};

private:
    Allocators allocators;

    /// IL program
    IL::Program &program;
};
