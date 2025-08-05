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

// Feature
#include <Features/DeviceCommands/Feature.h>
#include <Features/Descriptor/Feature.h>
#include <Features/DeviceCommands/CommandValidationProgram.h>
#include <Features/DeviceCommands/CommandValidationData.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/ShaderData/ShaderDataValidationCoverage.h>
#include <Backend/CommandContext.h>
#include <Backend/Command/CommandBuilder.h>

// Generated schema
#include <Schemas/Features/DeviceCommands.h>
#include <Schemas/Features/DeviceCommandConfig.h>

bool DeviceCommandsFeature::Install() {
    // Must have the export host
    auto exportHost = registry->Get<IShaderExportHost>();
    if (!exportHost) {
        return false;
    }

    // Allocate the shared export
    exportID = exportHost->Allocate<DeviceCommandInvalidArgumentMessage>();

    // Optional sguid host
    sguidHost = registry->Get<IShaderSGUIDHost>();

    // Coverage host buffers
    dataValidationCoverage = registry->Get<ShaderDataValidationCoverage>();

    // Create the validation program
    validationProgram = registry->New<CommandValidationProgram>(exportID);
    if (!validationProgram->Install()) {
        return false;
    }

    // OK
    return true;
}

FeatureHookTable DeviceCommandsFeature::GetHookTable() {
    FeatureHookTable table{};
    table.deviceCommand = BindDelegate(this, DeviceCommandsFeature::OnDeviceCommand);
    return table;
}

void DeviceCommandsFeature::Handle(const MessageStream *streams, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        ConstMessageStreamView view(streams[i]);

        // Visit all ordered messages
        for (ConstMessageStreamView<>::ConstIterator it = view.GetIterator(); it; ++it) {
            switch (it.GetID()) {
                case SetDeviceCommandInstrumentationConfigMessage::kID: {
                    config = *it.Get<SetDeviceCommandInstrumentationConfigMessage>();
                    break;
                }
            }
        }
    }
}

void DeviceCommandsFeature::OnDeviceCommand(CommandContext *context, const ResourceInfo& signature, const ResourceInfo& sourceCommand, const ResourceInfo& destCommand) {
    // Pass along validation data
    CommandValidationData validationData;
    validationData.dispatchGroupLimit = config.dispatchGroupLimit;

    // Dispatch the validator
    CommandBuilder builder(context->buffer);
    builder.SetShaderProgram(validationProgram->GetID());
    builder.SetDescriptorData(validationProgram->GetDataID(), validationData);
    builder.SetResource(validationProgram->GetSignatureID(), signature.token.puid);
    builder.SetResource(validationProgram->GetSourceCommandID(), sourceCommand.token.puid);
    builder.SetResource(validationProgram->GetDestCommandID(), destCommand.token.puid);
    builder.Dispatch(1, 1, 1);

    // Sync before device command
    builder.UAVBarrier();
}

FeatureInfo DeviceCommandsFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Device Commands";
    info.description = "Validation of indirect device commands against user configured limits";
    return info;
}
