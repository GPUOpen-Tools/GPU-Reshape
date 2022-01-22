#include <Assembler.h>

#include <Common/Assert.h>

#include <filesystem>
#include <sstream>
#include <map>

Assembler::Assembler(const AssembleInfo &assembleInfo, const Program &program) : assembleInfo(assembleInfo), program(program) {
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

    // Shader include
    includes << "#include \"" << assembleInfo.shaderPath << "\"\n";

    // Schema includes
    for (const std::string_view& schema : program.schemas) {
        includes << "#include <" << schema << ">\n";
    }

    // Replace
    testTemplate.Substitute("$INCLUDES", includes.str().c_str());

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
            fields << "\tint32_t " << attr.name << ";\n";

            tests << "\t\t\t\tREQUIRE_FORMAT(bucket->" << attr.name << " == it->" << attr.name << ", \"Message attribute " << attr.name << " is \" << it->" << attr.name << " << \" but expected \" << bucket->" << attr.name << ");\n";
        }

        // Generate messages
        for (const ProgramMessage &message: kv.second) {
            inits << "\t\t{\n";
            inits << "\t\t\t" << assembleInfo.program << "MessageInfo& msg = messages.emplace_back();\n";
            inits << "\t\t\tmsg.line = " << message.line << ";\n";
            inits << "\t\t\tmsg.count = " << message.count << ";\n";
            for (const ProgramMessageAttribute &attr: message.attributes) {
                inits << "\t\t\tmsg." << attr.name << " = " << attr.value << ";\n";
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
        fields << "\t" << assembleInfo.program << "MessageConstraint* " << kv.first << "Constraint{nullptr};\n";

        install << "\t\t" << kv.first << "Constraint = registry->New<" << assembleInfo.program << "MessageConstraint>();\n";
        install << "\t\t" << kv.first << "Constraint->Install();\n\n";
        install << "\t\tbridge->Register(" << kv.first << "Message::kID, " << kv.first << "Constraint);\n";

        validate << "\t\t" << kv.first << "Constraint->Validate();\n";
    }

    // Generate command
    commands << "\t\tdevice->Dispatch(commandBuffer, ";
    commands << program.invocation.groupCountX << ", ";
    commands << program.invocation.groupCountY << ", ";
    commands << program.invocation.groupCountZ;
    commands << ");\n";

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
            case ResourceType::Texture3D:
                types << "ResourceType::Texture3D";
                break;
            case ResourceType::RWTexture3D:
                types << "ResourceType::RWTexture3D";
                break;
        }
        types << ",\n";

        create << "\t\t";
        switch (resource.type) {
            default:
            ASSERT(false, "Invalid type");
                break;
            case ResourceType::Buffer:
                create << "BufferID resource" << i << " = device->CreateTexelBuffer(ResourceType::TexelBuffer, Backend::IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0);
                create << ", nullptr);\n";
                break;
            case ResourceType::RWBuffer:
                create << "BufferID resource" << i << " = device->CreateTexelBuffer(ResourceType::RWTexelBuffer, Backend::IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0);
                create << ", nullptr);\n";
                break;
            case ResourceType::Texture1D:
                create << "TextureID resource" << i << " = device->CreateTexture(ResourceType::Texture1D, Backend::IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0);
                create << "1, ";
                create << "1, ";
                create << "nullptr);\n";
                break;
            case ResourceType::RWTexture1D:
                create << "TextureID resource" << i << " = device->CreateTexture(ResourceType::RWTexture1D, Backend::IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0);
                create << "1, ";
                create << "1, ";
                create << "nullptr);\n";
                break;
            case ResourceType::Texture2D:
                create << "TextureID resource" << i << " = device->CreateTexture(ResourceType::Texture2D, Backend::IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0) << ", ";
                create << resource.initialization.sizes.at(1) << ", ";
                create << "1, ";
                create << "nullptr);\n";
                break;
            case ResourceType::RWTexture2D:
                create << "TextureID resource" << i << " = device->CreateTexture(ResourceType::RWTexture2D, Backend::IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0) << ", ";
                create << resource.initialization.sizes.at(1) << ", ";
                create << "1, ";
                create << "nullptr);\n";
                break;
            case ResourceType::Texture3D:
                create << "TextureID resource" << i << " = device->CreateTexture(ResourceType::Texture3D, Backend::IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0) << ", ";
                create << resource.initialization.sizes.at(1) << ", ";
                create << resource.initialization.sizes.at(2);
                create << ", nullptr);\n";
                break;
            case ResourceType::RWTexture3D:
                create << "TextureID resource" << i << " = device->CreateTexture(ResourceType::RWTexture3D, Backend::IL::Format::" << resource.format << ", ";
                create << resource.initialization.sizes.at(0) << ", ";
                create << resource.initialization.sizes.at(1) << ", ";
                create << resource.initialization.sizes.at(2);
                create << ", nullptr);\n";
                break;
        }

        set << "\t\t\tresource" << i << ",\n";
    }

    // Replace
    testTemplate.Substitute("$RESOURCES_CREATE", create.str().c_str());
    testTemplate.Substitute("$RESOURCES_TYPES", types.str().c_str());
    testTemplate.Substitute("$RESOURCES_SET", set.str().c_str());
}
