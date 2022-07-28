#pragma once

// Layer
#include <Backends/DX12/DX12.h>

// Common
#include <Common/IComponent.h>

// DXC
#include <DXC/dxcapi.h>

// System
#include <wrl/client.h>

class DXILSigner : public TComponent<DXILSigner> {
public:
    COMPONENT(DXILSigner);

    /// Deconstructor
    ~DXILSigner();

    /// Install this signer
    /// \return false if failed
    bool Install();

    /// Sign a DXIL blob
    /// \param code blob code
    /// \param length blob length
    /// \return false if failed
    bool Sign(void* code, uint64_t length);

private:
    /// Objects
    Microsoft::WRL::ComPtr<IDxcLibrary> library;
    Microsoft::WRL::ComPtr<IDxcValidator> validator;

private:
    /// Dynamic modules
    HMODULE dxilModule{nullptr};
    HMODULE dxCompilerModule{nullptr};
};
