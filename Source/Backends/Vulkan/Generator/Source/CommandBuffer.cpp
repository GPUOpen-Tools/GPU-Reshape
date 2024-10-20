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

#include "GenTypes.h"

// Std
#include <iostream>
#include <array>

bool Generators::CommandBuffer(const GeneratorInfo& info, TemplateEngine& templateEngine) {
    // Local lookup
    std::map<std::string, tinyxml2::XMLElement*> commandMap;
    
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

        // Is this an alias?
        const char* aliasName = command->Attribute("alias", nullptr);

        // Name of the command
        const char* name = nullptr;
        
        // Get underlying command
        if (aliasName) {
            // Assume alias command name
            name = command->Attribute("name", nullptr);

            // Resume prototype as if the base command
            command = commandMap.at(aliasName);
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

        // Get name from prototype if needed
        if (!name) {
            // Get the name
            tinyxml2::XMLElement *prototypeName = prototype->FirstChildElement("name");
            if (!prototypeName) {
                std::cerr << "Malformed command in line: " << command->GetLineNum() << ", prototype name not found" << std::endl;
                continue;
            }

            // Cache command
            name = prototypeName->GetText();
            commandMap[name] = command;
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
        if (!isHookCandidate || info.filter.excludedObjects.contains(name)) {
            continue;
        }

        // Add population, always pull all functions regardless of whitelist
        populate << "\tnext_" << name << " = reinterpret_cast<PFN_" << name << ">(";
        populate << "getProcAddr(device, \"" << name << "\")";
        populate << ");\n";

        // Prefix for the next call
        std::string namePrefix;

        // Check name
        const bool isHooked = info.hooks.count(name);
        const bool isWhitelisted = info.whitelist.count(name);

        // Determine name prefix
        if (isHooked) {
            namePrefix = "ProxyHook_";
        } else {
            namePrefix = "Hook_";
        }

        // Add get hook address, must be done after white listing
        gethookaddress << "\tif (!std::strcmp(\"" << name << "\", name)) {\n";
        gethookaddress << "\t\tif (!table || table->next_" << name << ") {\n";
        gethookaddress << "\t\t\treturn reinterpret_cast<PFN_vkVoidFunction>(" << namePrefix << name << ");\n";
        gethookaddress << "\t\t}\n";
        gethookaddress << "\t}\n\n";

        // If whitelisted and not hooked, don't generate anything
        if (!isHooked && isWhitelisted) {
            continue;
        }

        // Generate prototype
        hooks << prototypeResult->GetText() << " " << namePrefix << name << "(";

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
        if (info.hooks.count(name)) {
            hooks << "\tif (ApplyFeatureHook<FeatureHook_" << name << ">(\n";
            hooks << "\t\t" << wrappedObject << ",\n";
            hooks << "\t\t&" << wrappedObject << "->userContext,\n";
            hooks << "\t\t" << wrappedObject  << "->dispatchTable.featureBitSet_" << name << ",\n";
            hooks << "\t\t" << wrappedObject  << "->dispatchTable.featureHooks_" << name << "\n";
            hooks << "\t\t";

            // Skip first parameter
            tinyxml2::XMLElement* firstProxyParam = firstParam->NextSiblingElement("param");

            // Generate arguments
            parameterIndex = 0;
            for (tinyxml2::XMLElement *paramNode = firstProxyParam; paramNode; paramNode = paramNode->NextSiblingElement("param"), parameterIndex++) {
                // Get the name
                tinyxml2::XMLElement *paramName = paramNode->FirstChildElement("name");
                if (!paramName) {
                    std::cerr << "Malformed parameter in line: " << paramNode->GetLineNum() << ", name not found" << std::endl;
                    return false;
                }

                // Generate argument
                hooks << ", " << paramName->GetText();
            }

            hooks << "\n\t)) {\n";
            hooks << "\t\tCommitCommands(commandBuffer);\n";
            hooks << "\t}\n\n";
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
        if (isWhitelisted) {
            hooks << "Hook_" << name << "(";
        } else {
            hooks << wrappedObject << "->dispatchTable.next_" << name << "(";
        }

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
            if (unwrappingStates[parameterIndex] && !isWhitelisted)
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
