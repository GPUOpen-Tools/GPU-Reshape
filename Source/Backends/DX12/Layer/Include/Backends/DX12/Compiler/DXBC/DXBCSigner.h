#pragma once

// Layer
#include <Backends/DX12/DX12.h>

// Common
#include <Common/IComponent.h>

/// Function types
using PFN_DXBCSign = HRESULT(*)(BYTE* pData, UINT32 byteCount);

class DXBCSigner : public TComponent<DXBCSigner> {
public:
    COMPONENT(DXBCSigner);

    /// Deconstructor
    ~DXBCSigner();

    /// Install this signer
    /// \return false if failed
    bool Install();

    /// Sign a DXBC blob
    /// \param code blob code
    /// \param length blob length
    /// \return false if failed
    bool Sign(void* code, uint64_t length);

private:
    /// Proc
    PFN_DXBCSign dxbcSign{};

private:
    /// Dynamic module
    HMODULE module{nullptr};
};
