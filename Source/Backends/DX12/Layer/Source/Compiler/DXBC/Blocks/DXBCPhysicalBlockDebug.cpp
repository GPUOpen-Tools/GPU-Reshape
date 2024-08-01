// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#include <Backends/DX12/Compiler/DXBC/DXBCHeader.h>
#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockScan.h>
#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockType.h>
#include <Backends/DX12/Compiler/DXBC/MSF/MSFStructure.h>
#include <Backends/DX12/Compiler/DXIL/DXILDebugModule.h>
#include <Backends/DX12/Compiler/DXBC/Blocks/DXBCPhysicalBlockDebug.h>
#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXBC/DXBCParseContext.h>
#include <Backends/DX12/Compiler/DXParseJob.h>
#include <Backends/DX12/Compiler/DXBC/MSF/MSFParseContext.h>
#include <Backends/DX12/Controllers/PDBController.h>
#include <Backends/DX12/Compiler/DXBC/DXBCUtils.h>

// Common
#include <Common/FileSystem.h>
#include <Common/Containers/TrivialStackVector.h>

// Std
#include <fstream>
#include <istream>

DXBCPhysicalBlockDebug::DXBCPhysicalBlockDebug(const Allocators &allocators, IL::Program &program, DXBCPhysicalBlockTable &table) :
    DXBCPhysicalBlockSection(allocators, program, table),
    pdbScanner(allocators),
    pdbShaderSourceInfo(pdbScanner),
    pdbContainerContents(allocators) {

}

bool DXBCPhysicalBlockDebug::Parse(const DXParseJob& job) {
    // Get optional ILDB block
    DXBCPhysicalBlock *ildbBlock = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::ILDB);

    // If the lookup failed, check for ILDN, may be hosted externally
    if (DXBCPhysicalBlock *ildnBlock = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::ILDN); !ildbBlock && ildnBlock) {
        DXBCParseContext ctx(ildnBlock->ptr, ildnBlock->length);

        // Get debug name
        auto debugName = ctx.Consume<DXILShaderDebugName>();

        // Null terminated path
        auto path = ALLOCA_ARRAY(char, debugName.nameLength + 1u);
        std::memcpy(path, &ctx.Get<const char>(), sizeof(char) * debugName.nameLength);
        path[debugName.nameLength] = '\0';

        // Search for possible candidates
        PDBCandidateList candidates(allocators);
        job.pdbController->GetCandidateList(path, candidates);

        // Check all PDB candidates
        for (const std::string_view& candidate : candidates) {
            if ((ildbBlock = TryParsePDB(candidate))) {
                break;
            }
        }
    }

    // At this point fall back to the hash itself, try to see if any file matches it
    if (DXBCPhysicalBlock* hashBlock = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::ShaderHash); !ildbBlock && hashBlock) {
        auto hash = DXBCParseContext(hashBlock->ptr, hashBlock->length).Consume<DXILShaderHash>();

        // Hexify it!
        char hashStringBuffer[kDXBCShaderDigestStringLength];
        DXBCShaderDigestToString(hash.digest, hashStringBuffer);

        // Search for possible candidates
        PDBCandidateList candidates(allocators);
        job.pdbController->GetCandidateList(hashStringBuffer, candidates);

        // Check all PDB candidates
        for (const std::string_view& candidate : candidates) {
            if ((ildbBlock = TryParsePDB(candidate))) {
                break;
            }
        }
    }

    // The ILDB physical block contains the canonical program and debug information.
    // Unfortunately basing the main program off the ILDB is more trouble than it's worth,
    // as stripping the debug data after recompilation is quite troublesome.
    if (ildbBlock) {
        auto* dxilDebugModule = new(allocators, kAllocModuleDXILDebug) DXILDebugModule(allocators.Tag(kAllocModuleDXILDebug), pdbShaderSourceInfo);

        // Attempt to parse the module
        if (!dxilDebugModule->Parse(ildbBlock->ptr, ildbBlock->length)) {
            return false;
        }

        // Set interface
        table.debugModule = dxilDebugModule;

        // OK
        return true;
    }

    // No debug data, also ok
    return true;
}

DXBCPhysicalBlock * DXBCPhysicalBlockDebug::TryParsePDB(const std::string_view &path) {
    std::ifstream in(std::string(path), std::ios_base::binary);
    if (!in.good()) {
        return nullptr;
    }

    // Get file size
    in.seekg(0, std::ios_base::end);
    uint64_t size = in.tellg();
    in.seekg(0);

    // Stream data in
    std::vector<char> data(size);
    in.read(data.data(), size);

    // Try to parse the MSF file system
    MSFParseContext msfCtx(data.data(), data.size(), allocators);
    if (!msfCtx.Parse()) {
        return nullptr;
    }

    // Get root directory
    const MSFDirectory& directory = msfCtx.GetDirectory();

    // Validate hashes
    if (DXBCPhysicalBlock *hashBlock = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::ShaderHash)) {
        auto hash = DXBCParseContext(hashBlock->ptr, hashBlock->length).Consume<DXILShaderHash>();

        // Get pdb header
        auto* pdbHeader = directory.files[1].As<DXILPDBHeader>();

        // Validate header against pdb
        if (std::memcmp(&hash.digest, &pdbHeader->digest, sizeof(DXILDigest)) != 0) {
            return nullptr;
        }
    }

    // Steal the file contents
    const MSFFile& containerFile = directory.files[5];
    pdbContainerContents = std::move(containerFile.data);

    // Scan embedded data
    if (!pdbScanner.Scan(pdbContainerContents.data(), pdbContainerContents.size())) {
        return nullptr;
    }

    // Parse optional source info
    pdbShaderSourceInfo.Parse();
    
    // Finally, fetch the embedded ILDB block
    return pdbScanner.GetPhysicalBlock(DXBCPhysicalBlockType::ILDB);
}
