#include <Plugin.h>
#include <ShaderExportGenerator.h>
#include <Generators/MessageGenerator.h>

// Common
#include <Common/Linkage.h>

ShaderExportGenerator generator;

DLL_EXPORT_C bool Install(Registry* registry, GeneratorHost* host) {
    auto messageGenerator = registry->Get<MessageGenerator>();

    host->AddSchema(&generator);
    host->AddMessage("shader-export", &generator);
    host->AddMessage("shader-export", messageGenerator);

    registry->Add(&generator);
    return true;
}

DLL_EXPORT_C void Uninstall(Registry* registry) {
    registry->Remove(&generator);
}
