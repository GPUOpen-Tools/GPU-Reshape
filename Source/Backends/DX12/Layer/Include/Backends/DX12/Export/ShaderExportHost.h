#pragma once

// Backend
#include <Backend/IShaderExportHost.h>

// Std
#include <vector>

class ShaderExportHost : public IShaderExportHost {
public:
    ShaderExportID Allocate(const ShaderExportTypeInfo &typeInfo) override;
    void Enumerate(uint32_t *count, ShaderExportID *out) override;
    ShaderExportTypeInfo GetTypeInfo(ShaderExportID id) override;
    uint32_t GetBound() override;

private:
    struct ShaderExportInfo {
        ShaderExportTypeInfo typeInfo;
    };

    /// All exports
    std::vector<ShaderExportInfo> exports;
};
