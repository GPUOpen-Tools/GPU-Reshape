#pragma once

// Backend
#include <Backend/IShaderExportHost.h>

// Common
#include <Common/Allocator/Vector.h>

// Std
#include <vector>

class ShaderExportHost : public IShaderExportHost {
public:
    ShaderExportHost(const Allocators &allocators);

    ShaderExportID Allocate(const ShaderExportTypeInfo &typeInfo) override;
    void Enumerate(uint32_t *count, ShaderExportID *out) override;
    ShaderExportTypeInfo GetTypeInfo(ShaderExportID id) override;
    uint32_t GetBound() override;

private:
    struct ShaderExportInfo {
        ShaderExportTypeInfo typeInfo;
    };

    /// All exports
    Vector<ShaderExportInfo> exports;
};
