#include "GenTypes.h"

#include <iostream>
#include <array>

bool Generators::CommandBuffer(const GeneratorInfo& info, TemplateEngine& templateEngine) {
    // Get the commands
    tinyxml2::XMLElement *commands = info.registry->FirstChildElement("commands");
    if (!commands) {
        std::cerr << "Failed to find commands in registry" << std::endl;
        return false;
    }

    // Template streams
    std::stringstream populate;
    std::stringstream hooks;
    std::stringstream gethookaddress;

    // Process all commands
    for (tinyxml2::XMLNode *commandNode = commands->FirstChild(); commandNode; commandNode = commandNode->NextSibling()) {
        tinyxml2::XMLElement *command = commandNode->ToElement();

        // Is this a hook candidate?
        bool isHookCandidate = false;

        // Skip aliases
        if (command->Attribute("alias", nullptr)) {
            continue;
        }

        // Fiond the prototype definition
        tinyxml2::XMLElement *prototype = command->FirstChildElement("proto");
        if (!prototype) {
            std::cerr << "Malformed command in line: " << command->GetLineNum() << ", prototype not found" << std::endl;
            return false;
        }

        // Get the result
        tinyxml2::XMLElement *prototypeResult = prototype->FirstChildElement("type");
        if (!prototypeResult) {
            std::cerr << "Malformed command in line: " << command->GetLineNum() << ", prototype result not found" << std::endl;
            return false;
        }

        // Check return type
        isHookCandidate |= !std::strcmp(prototypeResult->GetText(), "VkCommandBuffer");

        // Get the name
        tinyxml2::XMLElement *prototypeName = prototype->FirstChildElement("name");
        if (!prototypeName) {
            std::cerr << "Malformed command in line: " << command->GetLineNum() << ", prototype name not found" << std::endl;
            return false;
        }

        // Whitelisted?
        if (info.whitelist.count(prototypeName->GetText())) {
            continue;
        }

        // First parameter
        tinyxml2::XMLElement *firstParam = command->FirstChildElement("param");

        // Unwrapping state
        std::array<bool, 128> unwrappingStates{};

        // Iterate all parameters
        uint32_t parameterIndex = 0;
        for (tinyxml2::XMLElement *paramNode = firstParam; paramNode; paramNode = paramNode->NextSiblingElement("param"), parameterIndex++) {
            bool &needsUnwrapping = unwrappingStates[parameterIndex];

            // Get the type
            tinyxml2::XMLElement *paramType = paramNode->FirstChildElement("type");
            if (!paramType) {
                std::cerr << "Malformed parameter in line: " << command->GetLineNum() << ", type not found" << std::endl;
                return false;
            }

            // Note: This is obviously not correct, it may be inside compound types and such, but it's sufficient for prototyping
            needsUnwrapping |= !std::strcmp(paramType->GetText(), "VkCommandBuffer");

            // Mark as candidate
            isHookCandidate |= needsUnwrapping;
        }

        // Skip if not interesting
        if (!isHookCandidate) {
            continue;
        }

        // Add population, always pull all functions regardless of whitelist
        populate << "\tnext_" << prototypeName->GetText() << " = reinterpret_cast<PFN_" << prototypeName->GetText() << ">(";
        populate << "getProcAddr(device, \"" << prototypeName->GetText() << "\")";
        populate << ");\n";

        // Add get hook address, must be done after white listing
        gethookaddress << "\tif (!std::strcmp(\"" << prototypeName->GetText() << "\", name))\n";
        gethookaddress << "\t\treturn reinterpret_cast<PFN_vkVoidFunction>(Hook_" << prototypeName->GetText() << ");\n";

        // Generate prototype
        hooks << prototypeResult->GetText() << " " << "Hook_" << prototypeName->GetText() << "(";

        // A wrapped object to inspect
        const char* wrappedObject{nullptr};

        // Generate parameters
        parameterIndex = 0;
        for (tinyxml2::XMLElement *paramNode = firstParam; paramNode; paramNode = paramNode->NextSiblingElement("param"), parameterIndex++) {
            // Get the type
            tinyxml2::XMLElement *paramType = paramNode->FirstChildElement("type");
            if (!paramType) {
                std::cerr << "Malformed parameter in line: " << paramNode->GetLineNum() << ", type not found" << std::endl;
                return false;
            }

            // Get the name
            tinyxml2::XMLElement *paramName = paramNode->FirstChildElement("name");
            if (!paramName) {
                std::cerr << "Malformed parameter in line: " << paramNode->GetLineNum() << ", name not found" << std::endl;
                return false;
            }

            // Comma
            if (paramNode != firstParam) {
                hooks << ", ";
            }

            // Qualifiers?
            if (const char* qualifiers = paramNode->GetText()) {
                hooks << qualifiers << " ";
            }

            // Accept as the wrapped object
            if (unwrappingStates[parameterIndex]) {
                wrappedObject = paramName->GetText();
            }

            // Generate parameter
            if (unwrappingStates[parameterIndex])
                hooks << "CommandBufferObject* ";
            else
                hooks << paramType->GetText();

            // Postfixes?
            for (auto postfix = paramType->NextSibling(); postfix != paramName; postfix = postfix->NextSibling()) {
                if (tinyxml2::XMLText* text = postfix->ToText()) {
                    hooks << text->Value();
                }
            }

            // Name
            hooks << " " << paramName->GetText();

            // Array?
            for (auto array = paramName->NextSibling(); array; array = array->NextSibling()) {
                if (tinyxml2::XMLText* text = array->ToText()) {
                    hooks << text->Value();
                }
            }
        }

        // Begin hook
        hooks << ") {\n";

        // Hooked?
        if (info.hooks.count(prototypeName->GetText())) {
            hooks << "\tApplyFeatureHook<FeatureHook_" << prototypeName->GetText() << ">(\n";
            hooks << "\t\t" << wrappedObject << "->dispatchTable.featureBitSet_" << prototypeName->GetText() << ",\n";
            hooks << "\t\t" << wrappedObject << "->dispatchTable.featureHooks_" << prototypeName->GetText() << ",\n";
            hooks << "\t\t";

            // Generate arguments
            parameterIndex = 0;
            for (tinyxml2::XMLElement *paramNode = firstParam; paramNode; paramNode = paramNode->NextSiblingElement("param"), parameterIndex++) {
                // Get the name
                tinyxml2::XMLElement *paramName = paramNode->FirstChildElement("name");
                if (!paramName) {
                    std::cerr << "Malformed parameter in line: " << paramNode->GetLineNum() << ", name not found" << std::endl;
                    return false;
                }

                // Comma
                if (paramNode != firstParam) {
                    hooks << ", ";
                }

                // Generate argument, unwrap if needed
                if (unwrappingStates[parameterIndex])
                    hooks << paramName->GetText() << "->object";
                else
                    hooks << paramName->GetText();
            }

            hooks << "\n\t);\n\n";
        }

        // Indent!
        hooks << "\t";

        // Anything to return?
        if (std::strcmp(prototypeResult->GetText(), "void") != 0) {
            hooks << "return ";
        }

        // Double check
        if (!wrappedObject) {
            std::cerr << "Wrapped object not found, unexpected error for command on line: " << command->GetLineNum() << std::endl;
            return false;
        }

        // Pass down the call chain
        hooks << wrappedObject << "->dispatchTable.next_" << prototypeName->GetText() << "(";

        // Generate arguments
        parameterIndex = 0;
        for (tinyxml2::XMLElement *paramNode = firstParam; paramNode; paramNode = paramNode->NextSiblingElement("param"), parameterIndex++) {
            // Get the type
            tinyxml2::XMLElement *paramType = paramNode->FirstChildElement("type");
            if (!paramType) {
                std::cerr << "Malformed parameter in line: " << paramNode->GetLineNum() << ", type not found" << std::endl;
                return false;
            }

            // Get the name
            tinyxml2::XMLElement *paramName = paramNode->FirstChildElement("name");
            if (!paramName) {
                std::cerr << "Malformed parameter in line: " << paramNode->GetLineNum() << ", name not found" << std::endl;
                return false;
            }

            // Comma
            if (paramNode != firstParam) {
                hooks << ", ";
            }

            // Generate argument, unwrap if needed
            if (unwrappingStates[parameterIndex])
                hooks << paramName->GetText() << "->object";
            else
                hooks << paramName->GetText();
        }

        // EOS
        hooks << ");\n";

        // End hook
        hooks << "}\n\n";
    }

    // Instantiate population
    if (!templateEngine.Substitute("$POPULATE", populate.str().c_str())) {
        std::cerr << "Bad template, failed to substitute $POPULATE" << std::endl;
        return false;
    }

    // Instantiate hooks
    if (!templateEngine.Substitute("$HOOKS", hooks.str().c_str())) {
        std::cerr << "Bad template, failed to substitute $HOOKS" << std::endl;
        return false;
    }

    // Instantiate hooks
    if (!templateEngine.Substitute("$GETHOOKADDRESS", gethookaddress.str().c_str())) {
        std::cerr << "Bad template, failed to substitute $GETHOOKADDRESS" << std::endl;
        return false;
    }

    // OK
    return true;
}
