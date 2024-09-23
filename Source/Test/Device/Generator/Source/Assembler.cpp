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

#include <Assembler.h>

// Common
#include <Common/Assert.h>

// Std
#include <filesystem>
#include <sstream>
#include <map>

Assembler::Assembler(const AssembleInfo &assembleInfo, const Program &program) : program(program), assembleInfo(assembleInfo) {
}

bool Assembler::Assemble(std::ostream &out) {
    // Load templates
    if (!testTemplate.Load((std::filesystem::path(assembleInfo.templatePath) / "Test.cppt").string().c_str()) ||
        !constraintTemplate.Load((std::filesystem::path(assembleInfo.templatePath) / "MessageConstraint.cppt").string().c_str())) {
        return false;
    }

    // Bucket by message type
    for (const ProgramMessage &message: program.messages) {
        buckets[message.type].push_back(message);
    }

    // Names
    testTemplate.SubstituteAll("$TEST_NAME", assembleInfo.program);
    testTemplate.SubstituteAll("$FEATURE_NAME", assembleInfo.feature);

    // Keys
    std::stringstream includes;
    std::stringstream defines;

    // Schema includes
    for (const std::string_view &schema: program.schemas) {
        includes << "#include <" << schema << ">\n";
    }

    // Vulkan?
#ifdef ENABLE_BACKEND_VULKAN
    defines << "#define ENABLE_BACKEND_VULKAN 1\n";
#endif // ENABLE_BACKEND_VULKAN

    // DX12?
#ifdef ENABLE_BACKEND_DX12
    defines << "#define ENABLE_BACKEND_DX12 1\n";
#endif // ENABLE_BACKEND_DX12

    // Replace
    testTemplate.Substitute("$INCLUDES", includes.str().c_str());
    testTemplate.Substitute("$DEFINES", defines.str().c_str());
    testTemplate.SubstituteAll("$EXECUTOR", program.executor.empty() ? "Ignore" : std::string(program.executor).c_str());
    testTemplate.SubstituteAll("$INLINE_EXECUTOR", program.executor.empty() ? "1" : "0");
    testTemplate.SubstituteAll("$SAFE_GUARDED", program.isSafeGuarded ? "1" : "0");

    // Assemble sections
    AssembleConstraints();
    AssembleResources();

    // Write out
    out << testTemplate.GetString();

    // OK
    return true;
}

void Assembler::AssembleConstraints() {
    std::stringstream ss;

    // Generate constraints
    for (auto &&kv: buckets) {
        constraintTemplate.SubstituteAll("$TEST_NAME", assembleInfo.program);
        constraintTemplate.SubstituteAll("$MESSAGE_TYPE", std::string(kv.first).c_str());

        // Keys
        std::stringstream fields;
        std::stringstream tests;
        std::stringstream inits;

        // Generate attributes
        for (const ProgramMessageAttribute &attr: kv.second[0].attributes) {
            fields << "\tstd::string " << attr.name << "Policy;\n";
            fields << "\tstd::function<bool(uint32_t)> " << attr.name << "Comparator;\n";

            tests << "\t\t\t\tREQUIRE_FORMAT(bucket->" << attr.name << "Comparator(it->" << attr.name << "), \"Message attribute comparison policy '\" << bucket->" << attr.name << "Policy << \"' failed on line \" << line);\n";
        }

        // Generate messages
        for (const ProgramMessage &message: kv.second) {
            inits << "\t\t{\n";
            inits << "\t\t\t" << assembleInfo.program << kv.first << "MessageInfo& msg = messages.emplace_back();\n";
            inits << "\t\t\tmsg.line = " << message.line << ";\n";
            inits << "\t\t\tmsg.policy = \"" << message.checkGenerator.contents << "\";\n";
            inits << "\t\t\tmsg.comparator = [](uint32_t x) { return " << message.checkGenerator.contents << "; };\n";

            for (const ProgramMessageAttribute &attr: message.attributes) {
                inits << "\t\t\tmsg." << attr.name << "Policy = \"" << attr.checkGenerator.contents << "\";\n";
                inits << "\t\t\tmsg." << attr.name << "Comparator = [](uint32_t x) { return " << attr.checkGenerator.contents << "; };\n";
            }

            inits << "\t\t}\n";
        }

        // Replace
        constraintTemplate.Substitute("$FIELDS", fields.str().c_str());
        constraintTemplate.Substitute("$TESTS", tests.str().c_str());
        constraintTemplate.Substitute("$INITS", inits.str().c_str());

        // Append
        ss << constraintTemplate.GetString();
        constraintTemplate.Reset();
    }

    // Assemble constraints
    testTemplate.Substitute("$CONSTRAINTS", ss.str().c_str());

    // Keys
    std::stringstream install;
    std::stringstream commands;
    std::stringstream validate;
    std::stringstream fields;

    // Generate messages
    for (auto &&kv: buckets) {
        fields << "\tComRef<" << assembleInfo.program << kv.first << "MessageConstraint> " << kv.first << "Constraint{nullptr};\n";

        install << "\t\t" << kv.first << "Constraint = registry->New<" << assembleInfo.program << kv.first << "MessageConstraint>();\n";
        install << "\t\t" << kv.first << "Constraint->Install();\n\n";
        install << "\t\tbridge->Register(" << kv.first << "Message::kID, " << kv.first << "Constraint);\n";

        validate << "\t\t" << kv.first << "Constraint->Validate();\n";
    }

    // Generate commands
    for (const ProgramInvocation& invocation : program.invocations) {
        commands << "\t\tdevice->Dispatch(commandBuffer, ";
        commands << invocation.groupCountX << ", ";
        commands << invocation.groupCountY << ", ";
        commands << invocation.groupCountZ;
        commands << ");\n";
    }

    // Replace
    testTemplate.Substitute("$CONSTRAINT_INSTALL", install.str().c_str());
    testTemplate.Substitute("$COMMANDS", commands.str().c_str());
    testTemplate.Substitute("$CONSTRAINT_VALIDATE", validate.str().c_str());
    testTemplate.Substitute("$CONSTRAINT_FIELDS", fields.str().c_str());
}

void Assembler::AssembleResources() {
    // Keys
    std::stringstream create;
    std::stringstream types;
    std::stringstream set;

    for (uint32_t i = 0; i < program.resources.size(); i++) {
        const Resource &resource = program.resources[i];

        types << "\t\t\t";
        switch (resource.type) {
            default:
            ASSERT(false, "Invalid type");
                break;
            case ResourceType::Buffer:
                types << "ResourceType::TexelBuffer";
                break;
            case ResourceType::RWBuffer:
                types << "ResourceType::RWTexelBuffer";
                break;
            case ResourceType::StructuredBuffer:
                types << "ResourceType::StructuredBuffer";
                break;
            case ResourceType::RWStructuredBuffer:
                types << "ResourceType::RWStructuredBuffer";
                break;
            case ResourceType::Texture1D:
                types << "ResourceType::Texture1D";
                break;
            case ResourceType::RWTexture1D:
                types << "ResourceType::RWTexture1D";
                break;
            case ResourceType::Texture2D:
                types << "ResourceType::Texture2D";
                break;
            case ResourceType::RWTexture2D:
                types << "ResourceType::RWTexture2D";
                break;
            case ResourceType::RWTexture2DArray:
                types << "ResourceType::RWTexture2DArray";
                break;
            case ResourceType::Texture3D:
                types << "ResourceType::Texture3D";
                break;
            case ResourceType::RWTexture3D:
                types << "ResourceType::RWTexture3D";
                break;
            case ResourceType::SamplerState:
                types << "ResourceType::SamplerState";
                break;
            case ResourceType::StaticSamplerState:
                types << "ResourceType::StaticSamplerState";
                break;
            case ResourceType::CBuffer:
                types << "ResourceType::CBuffer";
                break;
        }
        types << ",\n";

        if (resource.type != ResourceType::SamplerState && resource.type != ResourceType::StaticSamplerState) {
            create << "\t\tconst uint32_t data" << i << "[] = {";
            for (uint64_t data : resource.initialization.data) {
                create << data << ", ";
            }

            if (resource.initialization.data.empty()) {
                create << "0";
            }

            create << "};\n";
        }

        create << "\t\t";
        switch (resource.type) {
            default:
            ASSERT(false, "Invalid type");
                break;
            case ResourceType::Buffer:
                create << "BufferID resource" << i << " = device->CreateTexelBuffer(ResourceType::TexelBuffer, IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0);
                create << ", data" << i << ", " << resource.initialization.data.size() << " * sizeof(uint32_t));\n";
                break;
            case ResourceType::RWBuffer:
                create << "BufferID resource" << i << " = device->CreateTexelBuffer(ResourceType::RWTexelBuffer, IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0);
                create << ", data" << i << ", " << resource.initialization.data.size() << " * sizeof(uint32_t));\n";
                break;
            case ResourceType::StructuredBuffer:
                create << "BufferID resource" << i << " = device->CreateStructuredBuffer(ResourceType::StructuredBuffer, " << resource.structuredSize << ", ";
                create << resource.initialization.sizes.at(0);
                create << ", data" << i << ", " << resource.initialization.data.size() << " * sizeof(uint32_t));\n";
                break;
            case ResourceType::RWStructuredBuffer:
                create << "BufferID resource" << i << " = device->CreateStructuredBuffer(ResourceType::RWStructuredBuffer, " << resource.structuredSize << ", ";
                create << resource.initialization.sizes.at(0);
                create << ", data" << i << ", " << resource.initialization.data.size() << " * sizeof(uint32_t));\n";
                break;
            case ResourceType::Texture1D:
                create << "TextureID resource" << i << " = device->CreateTexture(ResourceType::Texture1D, IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0);
                create << "1, ";
                create << "1, ";
                create << "data" << i << ", " << resource.initialization.data.size() << ");\n";
                break;
            case ResourceType::RWTexture1D:
                create << "TextureID resource" << i << " = device->CreateTexture(ResourceType::RWTexture1D, IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0);
                create << "1, ";
                create << "1, ";
                create << "data" << i << ", " << resource.initialization.data.size() << " * sizeof(uint32_t));\n";
                break;
            case ResourceType::Texture2D:
                create << "TextureID resource" << i << " = device->CreateTexture(ResourceType::Texture2D, IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0) << ", ";
                create << resource.initialization.sizes.at(1) << ", ";
                create << "1, ";
                create << "data" << i << ", " << resource.initialization.data.size() << " * sizeof(uint32_t));\n";
                break;
            case ResourceType::RWTexture2D:
                create << "TextureID resource" << i << " = device->CreateTexture(ResourceType::RWTexture2D, IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0) << ", ";
                create << resource.initialization.sizes.at(1) << ", ";
                create << "1, ";
                create << "data" << i << ", " << resource.initialization.data.size() << " * sizeof(uint32_t));\n";
                break;
            case ResourceType::RWTexture2DArray:
                create << "TextureID resource" << i << " = device->CreateTexture(ResourceType::RWTexture2DArray, IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0) << ", ";
                create << resource.initialization.sizes.at(1) << ", ";
                create << resource.initialization.sizes.at(2) << ", ";
                create << "data" << i << ", " << resource.initialization.data.size() << " * sizeof(uint32_t));\n";
                break;
            case ResourceType::Texture3D:
                create << "TextureID resource" << i << " = device->CreateTexture(ResourceType::Texture3D, IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0) << ", ";
                create << resource.initialization.sizes.at(1) << ", ";
                create << resource.initialization.sizes.at(2) << ", ";
                create << "data" << i << ", " << resource.initialization.data.size() << " * sizeof(uint32_t));\n";
                break;
            case ResourceType::RWTexture3D:
                create << "TextureID resource" << i << " = device->CreateTexture(ResourceType::RWTexture3D, IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0) << ", ";
                create << resource.initialization.sizes.at(1) << ", ";
                create << resource.initialization.sizes.at(2) << ", ";
                create << "data" << i << ", " << resource.initialization.data.size() << " * sizeof(uint32_t));\n";
                break;
            case ResourceType::SamplerState:
            case ResourceType::StaticSamplerState:
                create << "SamplerID resource" << i << " = device->CreateSampler();\n";
                break;
            case ResourceType::CBuffer:
                create << "CBufferID resource" << i << " = device->CreateCBuffer(64, data" << i << ", " << resource.initialization.data.size() << " * sizeof(uint32_t));\n";
                break;
        }

        set << "\t\t\tresource" << i << ",\n";
    }

    if (program.resources.empty()) {
        types << "{}";
        set << "{}";
    }

    // Replace
    testTemplate.Substitute("$RESOURCES_CREATE", create.str().c_str());
    testTemplate.Substitute("$RESOURCES_TYPES", types.str().c_str());
    testTemplate.Substitute("$RESOURCES_SET", set.str().c_str());
    testTemplate.Substitute("$HAS_RESOURCES", program.resources.empty() ? "false" : "true");
}
