#include <Backends/Vulkan/Export/ShaderExportHost.h>

ShaderExportID ShaderExportHost::Allocate(const ShaderExportTypeInfo &typeInfo) {
    ShaderExportInfo& info = exports.emplace_back();
    info.typeInfo = typeInfo;

    // OK
    return static_cast<ShaderExportID>(exports.size() - 1);
}

void ShaderExportHost::Enumerate(uint32_t *count, ShaderExportID *out) {
    if (out) {
        for (uint32_t i = 0; i < *count; i++) {
            out[i] = i;
        }
    } else {
        *count = static_cast<uint32_t>(exports.size());
    }
}

uint32_t ShaderExportHost::GetBound() {
    return static_cast<uint32_t>(exports.size());
}

ShaderExportTypeInfo ShaderExportHost::GetTypeInfo(ShaderExportID id) {
    return exports.at(id).typeInfo;
}
