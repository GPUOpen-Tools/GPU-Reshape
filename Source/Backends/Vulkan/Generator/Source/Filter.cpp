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

// Generator
#include "GenTypes.h"

// Std
#include <iostream>

/// Check if a tag is excluded
static bool IsExcluded(const std::string& tags) {
    // If the tag is only dependent on safety-critical, the core headers will not have them
    return tags == "vulkansc";
}

/// Exclude all the extensions from a node
/// \param extension node to traverse
/// \param primaryIsExcluded if true, all named objects from this node onwards is excluded
/// \param out destination filter
static void ExcludeRegistryExtensionNode(tinyxml2::XMLElement* extension, bool primaryIsExcluded, FilterInfo& out) {
    for (tinyxml2::XMLNode *childNode = extension->FirstChild(); childNode; childNode = childNode->NextSibling()) {
        tinyxml2::XMLElement *child = childNode->ToElement();
        if (!child) {
            continue;
        }

        // If named, mark as excluded
        if (primaryIsExcluded) {
            if (const char* name = child->Attribute("name", nullptr)) {
                out.excludedObjects.insert(name);
            }
        }

        // Exclude all nested items
        if (!std::strcmp(child->Name(), "require")) {
            bool requireIsExcluded = primaryIsExcluded;

            // May be excluded by the API requirements
            if (const char* api = child->Attribute("api")) {
                requireIsExcluded |= IsExcluded(api);
            }
            
            ExcludeRegistryExtensionNode(child, requireIsExcluded, out);
        }
    }
}

/// Filter all extension structures for exclusion
/// \param registry registry to filter
/// \param out destination filter
/// \return success state
bool FilterRegistryExtensions(tinyxml2::XMLElement* registry, FilterInfo& out) {
    tinyxml2::XMLElement *extensions = registry->FirstChildElement("extensions");
    if (!extensions) {
        std::cerr << "Failed to find extensions in registry" << std::endl;
        return false;
    }

    // Visit all extensions
    for (tinyxml2::XMLNode *extensionNode = extensions->FirstChild(); extensionNode; extensionNode = extensionNode->NextSibling()) {
        tinyxml2::XMLElement *extension = extensionNode->ToElement();

        // Ignore non-conditional extensions
        const char* supported = extension->Attribute("supported", nullptr);
        if (!supported) {
            continue;
        }

        // Mark all nodes as excluded
        ExcludeRegistryExtensionNode(extension, IsExcluded(supported), out);
    }

    return true;
}

/// Filter all feature structures for exclusion
/// \param registry registry to filter
/// \param out destination filter
/// \return success state
bool FilterRegistryFeatures(tinyxml2::XMLElement* registry, FilterInfo& out) {
    for (tinyxml2::XMLNode *childNode = registry->FirstChild(); childNode; childNode = childNode->NextSibling()) {
        tinyxml2::XMLElement *child = childNode->ToElement();
        if (!child || std::strcmp(child->Name(), "feature")) {
            continue;
        }

        // Ignore API-less features
        const char* api = child->Attribute("api", nullptr);
        if (!api) {
            continue;
        }

        // Mark all nodes as excluded
        ExcludeRegistryExtensionNode(child, IsExcluded(api), out);
    }

    return true;
}

bool FilterRegistry(tinyxml2::XMLElement *registry, FilterInfo &out) {
    return
        FilterRegistryExtensions(registry, out) &&
        FilterRegistryFeatures(registry, out);
}
