// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXParseJob.h>
#include <Backends/DX12/Compiler/Tags.h>
#include <Backends/DX12/Compiler/DXCompileJob.h>
#include <Backends/DX12/Compiler/Diagnostic/DiagnosticType.h>

// DXIL Extension
#include <Backends/DX12/Compiler/DXIL/DXILModule.h>
#include <Backends/DX12/Compiler/DXIL/DXILDebugModule.h>

// Common
#include <Common/Allocators.h>

DXBCPhysicalBlockTable::DXBCPhysicalBlockTable(const Allocators &allocators, IL::Program &program) :
    scan(allocators),
    shader(allocators, program, *this),
    pipelineStateValidation(allocators, program, *this),
    rootSignature(allocators, program, *this),
    featureInfo(allocators, program, *this),
    inputSignature(allocators, program, *this),
    outputSignature(allocators, program, *this),
    debug(allocators, program, *this),
    allocators(allocators),
    program(program) {
    /* */
}

DXBCPhysicalBlockTable::~DXBCPhysicalBlockTable() {
    if (dxilModule) {
        destroy(dxilModule, allocators);
    }
    
    if (debugModule && !shallowDebug) {
        destroy(debugModule, allocators);
    }
}

bool DXBCPhysicalBlockTable::Parse(const DXParseJob& job) {
    if (!scan.Scan(job.byteCode, job.byteLength)) {
        return false;
    }

    // Parse blocks
    shader.Parse();
    pipelineStateValidation.Parse();
    rootSignature.Parse();
    featureInfo.Parse();
    inputSignature.Parse();
    outputSignature.Parse();

    // Parse canonical program
    if (DXBCPhysicalBlock *dxilBlock = scan.GetPhysicalBlock(DXBCPhysicalBlockType::DXIL)) {
        dxilModule = new(allocators, kAllocModuleDXBC) DXILModule(allocators.Tag(kAllocModuleDXBC), &program);

        // Create new job
        DXParseJob dxilJob = job;
        dxilJob.byteCode = dxilBlock->ptr;
        dxilJob.byteLength = dxilBlock->length;

        // Attempt to parse the module
        if (!dxilModule->Parse(dxilJob)) {
            return false;
        }
    }

    // Parse debug blocks
    if (!debug.Parse(job)) {
        return false;
    }

    // OK
    return true;
}

bool DXBCPhysicalBlockTable::Compile(const DXCompileJob &job) {    
    // DXIL?
    if (dxilModule) {
        DXBCPhysicalBlock *block = scan.GetPhysicalBlock(DXBCPhysicalBlockType::DXIL);

        // Create new job
        DXCompileJob dxilJob = job;
        inputSignature.CompileDXILCompatability(dxilJob);
        outputSignature.CompileDXILCompatability(dxilJob);

        // Attempt to compile contained module
        if (!dxilModule->Compile(dxilJob, block->stream)) {
            job.messages.Add(DiagnosticType::ShaderInternalCompilerError);
            return false;
        }
    } else {
        // non-DXIL DXBC recompilation is not supported yet
        job.messages.Add(DiagnosticType::ShaderNativeDXBCNotSupported);
        return false;
    }

    // Compile validation
    pipelineStateValidation.Compile();
    rootSignature.Compile();
    featureInfo.Compile();
    inputSignature.Compile();
    outputSignature.Compile();

    // OK
    return true;
}

bool DXBCPhysicalBlockTable::Stitch(const DXCompileJob& job, DXStream &out, bool sign) {
    return scan.Stitch(job, out, sign);
}

void DXBCPhysicalBlockTable::CopyTo(DXBCPhysicalBlockTable &out) {
    scan.CopyTo(out.scan);

    // Copy blocks
    pipelineStateValidation.CopyTo(out.pipelineStateValidation);
    rootSignature.CopyTo(out.rootSignature);
    featureInfo.CopyTo(out.featureInfo);
    inputSignature.CopyTo(out.inputSignature);
    outputSignature.CopyTo(out.outputSignature);

    // Keep the debug interface
    out.debugModule = debugModule;
    out.shallowDebug = true;

    // Copy submodule if present
    if (dxilModule) {
        out.dxilModule = new(allocators, kAllocModuleDXIL) DXILModule(allocators.Tag(kAllocModuleDXIL), &out.program);

        // Copy to new module
        dxilModule->CopyTo(out.dxilModule);
    }
}
