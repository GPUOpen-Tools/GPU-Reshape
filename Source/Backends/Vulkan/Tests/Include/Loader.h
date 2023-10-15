// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

#pragma once

// Third party
#include <catch2/catch.hpp>
#include <vulkan/vulkan.h>

// Backend
#include <Backend/Environment.h>

// Bridge
#include <Bridge/IBridge.h>

// Std
#include <vector>

/// Default test loader
class Loader {
public:
	Loader();
    ~Loader();

    /// Create an instance with the given extensions
    void CreateInstance();

    /// Create a device with the given extensions
    void CreateDevice();

    /// Does this loader supports a given layer
    [[nodiscard]]
    bool SupportsInstanceLayer(const char* name) const;

    /// Does this loader supports a given extension
    [[nodiscard]]
    bool SupportsInstanceExtension(const char* name) const;

    /// Does this loader supports a given extension
    [[nodiscard]]
    bool SupportsDeviceExtension(const char* name) const;

    /// Add an instance layer
    bool AddInstanceLayer(const char* name);

    /// Add an instance extension
    bool AddInstanceExtension(const char* name);

    /// Add a device extension, instance must be created
    bool AddDeviceExtension(const char* name);

    /// Enable validation layers
    /// \return success state
    bool EnableValidation();

	/// Get the physical device
	[[nodiscard]]
	VkPhysicalDevice GetPhysicalDevice() const {
		return physicalDevice;
	}

    /// Get the instance
    [[nodiscard]]
    VkInstance GetInstance() const {
        return instance;
    }

	/// Get the device
	[[nodiscard]]
	VkDevice GetDevice() const {
		return device;
	}

    /// Get the primary queue family
    [[nodiscard]]
    uint32_t GetPrimaryQueueFamily() const {
        return queueFamilyIndex;
    }

    /// Get the primary queue
    [[nodiscard]]
    VkQueue GetPrimaryQueue() const {
        return queue;
    }

    /// Get the registry
    [[nodiscard]]
    Registry* GetRegistry() {
        return environment.GetRegistry();
    }

private:
	VkInstance       instance      {VK_NULL_HANDLE};
	VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
	VkDevice         device        {VK_NULL_HANDLE};
    VkQueue          queue         {VK_NULL_HANDLE};

    VkDebugUtilsMessengerEXT debugMessenger{VK_NULL_HANDLE};

    uint32_t queueFamilyIndex{0xFFFFFFFF};

private:
    bool enableValidation{false};

    std::vector<VkLayerProperties>     instanceLayers;
    std::vector<VkExtensionProperties> instanceExtensions;
    std::vector<VkExtensionProperties> deviceExtensions;

    std::vector<const char*> enabledInstanceLayers;
    std::vector<const char*> enabledInstanceExtensions;
    std::vector<const char*> enabledDeviceExtensions;

private:
    Backend::Environment environment;
};
