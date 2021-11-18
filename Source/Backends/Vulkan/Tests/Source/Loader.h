#pragma once

// Third party
#include <catch2/catch.hpp>
#include <vulkan/vulkan.h>

// Common
#include <Common/Registry.h>

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
        return &registry;
    }

    /// Get the bridge
    [[nodiscard]]
    IBridge* GetBridge() {
        return bridge;
    }

private:
	VkInstance       instance      {VK_NULL_HANDLE};
	VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
	VkDevice         device        {VK_NULL_HANDLE};
    VkQueue          queue         {VK_NULL_HANDLE};

    uint32_t queueFamilyIndex{0xFFFFFFFF};

private:
    std::vector<VkLayerProperties>     instanceLayers;
    std::vector<VkExtensionProperties> instanceExtensions;
    std::vector<VkExtensionProperties> deviceExtensions;

    std::vector<const char*> enabledInstanceLayers;
    std::vector<const char*> enabledInstanceExtensions;
    std::vector<const char*> enabledDeviceExtensions;

private:
    Registry registry;

    IBridge* bridge;
};
