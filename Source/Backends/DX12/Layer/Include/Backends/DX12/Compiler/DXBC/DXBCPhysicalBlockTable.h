#pragma once

// Layer
#include "DXBCPhysicalBlockScan.h"
#include "Blocks/DXBCPhysicalBlockShader.h"
#include "Blocks/DXBCPhysicalBlockPipelineStateValidation.h"
#include "Blocks/DXBCPhysicalBlockRootSignature.h"
#include "Blocks/DXBCPhysicalBlockFeatureInfo.h"
#include "Blocks/DXBCPhysicalBlockInputSignature.h"
#include "Blocks/DXBCPhysicalBlockDebug.h"

// Forward declarations
struct DXParseJob;
struct DXCompileJob;
struct DXStream;
class DXILModule;
class IDXDebugModule;

/// Complete physical block table
struct DXBCPhysicalBlockTable {
    DXBCPhysicalBlockTable(const Allocators &allocators, IL::Program &program);
    ~DXBCPhysicalBlockTable();

    /// Scan the DXBC bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    bool Parse(const DXParseJob& job);

    /// Compile the table
    /// \param job the job to compile against
    /// \return success state
    bool Compile(const DXCompileJob& job);

    /// Stitch the compiled table
    /// \param out destination stream
    void Stitch(const DXCompileJob& job, DXStream &out);

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
    DXBCPhysicalBlockInputSignature inputSignature;
    DXBCPhysicalBlockDebug debug;

    /// DXBC containers can host DXIL data
    DXILModule* dxilModule{nullptr};

    /// Optional debug data
    IDXDebugModule* debugModule{nullptr};

private:
    /// Is the debugging module owned?
    bool shallowDebug{false};

private:
    Allocators allocators;

    /// IL program
    IL::Program &program;
};
