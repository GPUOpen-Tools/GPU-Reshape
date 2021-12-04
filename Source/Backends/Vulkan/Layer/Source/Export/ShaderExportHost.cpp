#include <Backends/Vulkan/Export/ShaderExportHost.h>

ShaderExportID ShaderExportHost::Allocate(const ShaderExportTypeInfo &typeInfo) {
    ShaderExportInfo& info = exports.emplace_back();
    info.typeInfo = typeInfo;

    // OK
    return static_cast<ShaderExportID>(exports.size() - 1);
}
