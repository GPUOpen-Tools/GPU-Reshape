// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

#include <Backends/DX12/Compiler/DXIL/DXILModule.h>
#include <Backends/DX12/Compiler/DXParseJob.h>
#include <Backends/DX12/Compiler/Tags.h>

DXILModule::DXILModule(const Allocators &allocators) : DXILModule(allocators, new(allocators, kAllocModuleDXIL) IL::Program(allocators, 0x0)) {
    nested = false;
}

DXILModule::DXILModule(const Allocators &allocators, IL::Program *program) :
    table(allocators, *program),
    program(program),
    allocators(allocators) {
    /* */
}

DXILModule::~DXILModule() {
    if (!nested) {
        destroy(program, allocators);
    }
}

IDXModule* DXILModule::Copy() {
    return nullptr;
}

bool DXILModule::Parse(const DXParseJob& job) {
    IL::CapabilityTable &capabilities = program->GetCapabilityTable();

    // LLVM integers are not unique to the sign, exposed through the operations instead
    capabilities.integerSignIsUnique = false;

    // Parse byte code
    return table.Parse(job.byteCode, job.byteLength);
}

IL::Program *DXILModule::GetProgram() {
    return program;
}

GlobalUID DXILModule::GetInstrumentationGUID() {
    return {};
}

bool DXILModule::Compile(const DXCompileJob& job, DXStream& out) {
    // Try to recompile for the given job
    if (!table.Compile(job)) {
        return false;
    }

    // Stitch to the program
    table.Stitch(out);

    // OK!
    return true;
}

void DXILModule::CopyTo(DXILModule *out) {
    table.CopyTo(out->table);
}

IDXDebugModule *DXILModule::GetDebug() {
    // No debug data
    return nullptr;
}

const char * DXILModule::GetLanguage() {
    return "DXIL";
}
