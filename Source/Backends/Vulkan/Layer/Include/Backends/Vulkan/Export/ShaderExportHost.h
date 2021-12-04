#pragma once

// Backend
#include <Backend/IShaderExportHost.h>

// Std
#include <vector>

class ShaderExportHost : public IShaderExportHost {
public:
    ShaderExportID Allocate(const ShaderExportTypeInfo &typeInfo) override;

private:
    struct ShaderExportInfo {
        ShaderExportTypeInfo typeInfo;
    };

    std::vector<ShaderExportInfo> exports;
};
