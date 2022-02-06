#include <Plugin.h>
#include <ShaderExportGenerator.h>
#include <Generators/MessageGenerator.h>

// Common
#include <Common/Linkage.h>

ComRef<ShaderExportGenerator> generator;

DLL_EXPORT_C bool Install(Registry* registry, GeneratorHost* host) {
    auto messageGenerator = registry->Get<MessageGenerator>();

    generator = registry->AddNew<ShaderExportGenerator>();

    host->AddSchema(generator);
    host->AddMessage("shader-export", generator);
    host->AddMessage("shader-export", messageGenerator);
    return true;
}

DLL_EXPORT_C void Uninstall(Registry* registry) {
    registry->Remove(generator);
}
