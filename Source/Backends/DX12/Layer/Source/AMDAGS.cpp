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

// Layer
#include <Backends/DX12/States/AGSState.h>
#include <Backends/DX12/Device.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/Layer.h>

// Global state
// AGS states are not wrapped to avoid detour pollution
std::mutex                 AGSState::Mutex;
std::map<void*, AGSState*> AGSState::Table;

AGSReturnCode HookAMDAGSInitialize(int agsVersion, const AGSConfiguration* config, AGSContext** context, AGSGPUInfo* gpuInfo) {
    Allocators allocators = {};
    
    // Pass down callchain
    AGSReturnCode code = D3D12GPUOpenFunctionTableNext.next_AMDAGSInitialize(agsVersion, config, context, gpuInfo);
    if (code != AGS_SUCCESS) {
        return code;
    }

    // Create state
    auto state = AGSState::Add(*context, new (allocators) AGSState());
    state->allocators = allocators;

    // Unpack version
    state->versionMajor = (agsVersion >> 22) & 0x3FF;
    state->versionMinor = (agsVersion >> 12) & 0x3FF;
    state->versionPatch = (agsVersion >> 00) & 0x3FF;

    // OK
    return AGS_SUCCESS;
}

DX12_C_LINKAGE AGSReturnCode HookAMDAGSDeinitialize(AGSContext* context) {
    // Pass down callchain
    AGSReturnCode code = D3D12GPUOpenFunctionTableNext.next_AMDAGSDeinitialize(context);
    if (code != AGS_SUCCESS) {
        return code;
    }

    // Free associated state
    if (AGSState* state = AGSState::Get(context)) {
        destroy(state, state->allocators);
        AGSState::Remove(state);
    }

    // OK
    return AGS_SUCCESS;
}

AGSReturnCode HookAMDAGSCreateDevice(AGSContext* context, const AGSDX12DeviceCreationParams* creationParams, const AGSDX12ExtensionParams* extensionParams, AGSDX12ReturnedParams* returnedParams) {
    // Setup SDK runtime
    D3D12GPUOpenSDKRuntime skdRuntime{};
    skdRuntime.isAMDAGS = true;

    // Keep track of the reserved spaces
    if (extensionParams && extensionParams->uavSlot != 0) {
        skdRuntime.AMDAGS.reservedUavSpace = 0;
        skdRuntime.AMDAGS.reservedUavSlot = extensionParams->uavSlot;
    } else {
        skdRuntime.AMDAGS.reservedUavSpace = AGS_DX12_SHADER_INTRINSICS_SPACE_ID;
        skdRuntime.AMDAGS.reservedUavSlot  = 0;
    }

    // Create with base interface
    AGSDX12DeviceCreationParams params = *creationParams;
    params.pAdapter = UnwrapObject(params.pAdapter);
    params.iid = __uuidof(ID3D12Device);

    // Try to enable for faster instrumentation
    ConditionallyEnableExperimentalMode();

    // Pass down callchain
    AGSReturnCode code = D3D12GPUOpenFunctionTableNext.next_AMDAGSCreateDevice(context, &params, extensionParams, returnedParams);
    if (code != AGS_SUCCESS) {
        return code;
    }

    // Queried device
    decltype(AGSDX12ReturnedParams::pDevice) device;

    // AGS *may* internally call the hooked device creation, double the wrap double the trouble
    if (DeviceState* wrap{nullptr}; SUCCEEDED(returnedParams->pDevice->QueryInterface(__uuidof(DeviceState), reinterpret_cast<void**>(&wrap)))) {
        // Query wrapped to expected interface
        if (FAILED(returnedParams->pDevice->QueryInterface(creationParams->iid, reinterpret_cast<void**>(&device)))) {
            return AGS_FAILURE;
        }

        // Parent caller implicitly adds a reference to the object, release the fake reference
        returnedParams->pDevice->Release();

        // OK
        returnedParams->pDevice = device;
        return AGS_SUCCESS;
    }

    // Create wrapper, iid dictated by creation parameters
    HRESULT hr = D3D12CreateDeviceGPUOpen(
        reinterpret_cast<ID3D12Device*>(returnedParams->pDevice),
        creationParams->pAdapter,
        creationParams->FeatureLevel,
        creationParams->iid,
        reinterpret_cast<void**>(&device),
        skdRuntime,
        D3D12DeviceGPUOpenGPUReshapeInfo ? &*D3D12DeviceGPUOpenGPUReshapeInfo : nullptr
    );
    if (FAILED(hr)) {
        return AGS_FAILURE;
    }

    // OK
    returnedParams->pDevice = device;
    return AGS_SUCCESS;
}
