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

#include <Backends/DX12/Compiler/ShaderCompilerDebug.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Export/ShaderExportDescriptorAllocator.h>
#include <Backends/DX12/ShaderData/ShaderDataHost.h>
#include <Backends/DX12/Compiler/DXBC/DXBCModule.h>
#include <Backends/DX12/Compiler/DXCompileJob.h>
#include <Backends/DX12/Modules/InbuiltTemplateModuleD3D12.h>
#include <Backends/DX12/ShaderProgram/ShaderProgramHost.h>
#include <Backends/DX12/Compiler/DXIL/DXILSigner.h>
#include <Backends/DX12/Compiler/DXBC/DXBCSigner.h>
#include <Backends/DX12/Export/ShaderExportHost.h>
#include <Backends/DX12/Compiler/DXParseJob.h>
#include <Backends/DX12/RootSignature.h>

// Backend
#include <Backend/ShaderProgram/IShaderProgram.h>

// Common
#include <Common/Registry.h>
#include <Common/GlobalUID.h>

ShaderProgramHost::ShaderProgramHost(DeviceState* device) :
    programs(device->allocators.Tag(kAllocShaderProgram)),
    freeIndices(device->allocators.Tag(kAllocShaderProgram)),
    shaderData(device->allocators.Tag(kAllocShaderProgram)),
    device(device) {

}

bool ShaderProgramHost::Install() {
    templateModule = new(registry->GetAllocators(), kAllocInstrumentation) DXBCModule(allocators, 0ull, GlobalUID::New());

    // Prepare job
    DXParseJob job;
    job.byteCode = reinterpret_cast<const uint32_t*>(kSPIRVInbuiltTemplateModuleD3D12);
    job.byteLength = static_cast<uint32_t>(sizeof(kSPIRVInbuiltTemplateModuleD3D12));
    job.pdbController = device->pdbController;

    // Attempt to parse template data
    if (!templateModule->Parse(job)) {
        return false;
    }

    // Optional debug
    debug = registry->Get<ShaderCompilerDebug>();

    // OK
    return true;
}

bool ShaderProgramHost::CreateRootSignature() {
    D3D12_ROOT_SIGNATURE_DESC1 desc{};

    // Instrument empty signature
    ID3DBlob* blob;
    if (FAILED(SerializeRootSignature(device, D3D_ROOT_SIGNATURE_VERSION_1_1, desc, &blob, &rootBindingInfo, &rootPhysicalMapping, nullptr))) {
        return false;
    }

    // Create state
    if (FAILED(device->object->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&rootSignature)))) {
        return false;
    }

    return true;
}

bool ShaderProgramHost::InstallPrograms() {
    // Get the signers
    auto dxilSigner = registry->Get<DXILSigner>();
    auto dxbcSigner = registry->Get<DXBCSigner>();

    // Create shared root signature
    if (!CreateRootSignature()) {
        return false;
    }

    // Get the export host
    auto shaderDataHost = registry->Get<ShaderDataHost>();

    // Get number of resources
    uint32_t resourceCount;
    shaderDataHost->Enumerate(&resourceCount, nullptr, ShaderDataType::All);

    // Fill resources
    shaderData.resize(resourceCount);
    shaderDataHost->Enumerate(&resourceCount, shaderData.data(), ShaderDataType::All);

    // Get the export host
    auto exportHost = registry->Get<IShaderExportHost>();

    // Get number of exports
    uint32_t exportCount;
    exportHost->Enumerate(&exportCount, nullptr);

    // Create all programs
    for (ProgramEntry& entry : programs) {
        if (!entry.program) {
            continue;
        }

        // Copy the template module
        entry.module = templateModule->Copy();

        // Get user map
        IL::ShaderDataMap& shaderDataMap = entry.module->GetProgram()->GetShaderDataMap();

        // Add resources
        for (const ShaderDataInfo& info : shaderData) {
            shaderDataMap.Add(info);
        }

        // Finally, inject the host program8
        entry.program->Inject(*entry.module->GetProgram());

        // Describe job
        DXCompileJob compileJob{};
        compileJob.instrumentationKey.bindingInfo = rootBindingInfo;
        compileJob.instrumentationKey.physicalMapping = rootPhysicalMapping;
        compileJob.streamCount = exportCount;
        compileJob.dxilSigner = dxilSigner;
        compileJob.dxbcSigner = dxbcSigner;

        // Attempt to recompile the module
        DXStream stream(allocators);
        if (!entry.module->Compile(compileJob, stream)) {
            return false;
        }

        // Debug
        if (debug) {
            // Allocate path
            std::filesystem::path debugPath = debug->AllocatePath("program");

            // Dump source
            debug->Add(
                debugPath,
                "instrumented",
                entry.module
            );
        }

        // Setup the compute state
        D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc{};
        computeDesc.CS.pShaderBytecode = stream.GetData();
        computeDesc.CS.BytecodeLength = stream.GetByteSize();
        computeDesc.pRootSignature = rootSignature;

        // Finally, create the pipeline
        HRESULT result = device->object->CreateComputePipelineState(&computeDesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&entry.pipeline));
        if (FAILED(result)) {
            return false;
        }
    }

    // OK
    return true;
}

ShaderProgramID ShaderProgramHost::Register(const ComRef<IShaderProgram> &program) {
    // Allocate identifier
    ShaderProgramID id;
    if (freeIndices.empty()) {
        id = static_cast<uint32_t>(programs.size());
        programs.emplace_back();
    } else {
        id = freeIndices.back();
        freeIndices.pop_back();
    }

    // Populate entry
    ProgramEntry& entry = programs[id];
    entry.program = program;

    // OK
    return id;
}

void ShaderProgramHost::Deregister(ShaderProgramID program) {
    ProgramEntry& entry = programs[program];

    // Release the module
    destroy(entry.module, registry->GetAllocators());

    // Cleanup entry
    entry = {};

    // Mark as free
    freeIndices.push_back(program);
}
