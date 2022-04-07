#pragma once
#ifndef GALAXYSAILING_VULKAN_VG_RASTERIZER_H_
#define GALAXYSAILING_VULKAN_VG_RASTERIZER_H_

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <string>

#include "../vk/vk_device.h"
#include "../vk/vk_swapchain.h"
#include "../vk/vk_util.h"
#include "../vk/vk_debug.h"

namespace Galaxysailing {

class VulkanVGRasterizerBase{
// ----------------------------- vulkan function -----------------------------
protected:
    void initVulkan();

	// prepares all Vulkan resources and functions
    virtual void prepare();

	virtual void getEnabledFeatures();

// ----------------------------- vulkan builder function ---------------------
private:
    bool createInstance();

    void initSwapchain();
    void createCommandPool();
    void setupSwapChain();
    void createCommandBuffers();
    void createSynchronizationPrimitives();
    void setupRenderPass();
    void createPipelineCache();
    void setupFrameBuffer();

	void windowResize();

// ----------------------------- rasterizer fields ---------------------------
protected:

    bool _prepared = false;

    struct Settings {
        // Activates validation layers (and message output) when set to true
        bool validation = true;
        // Set to true if v-sync will be forced for the swapchain
        bool vsync = false;
		// Enable UI overlay
		bool overlay = true;
    } settings;

    uint32_t _width, _height;
    GLFWwindow* _window;

// ----------------------------- vulkan fields -------------------------------
protected:
    
    uint32_t _apiVersion = VK_API_VERSION_1_0;

	// Vulkan instance, stores all per-application states
	VkInstance _instance;
	std::vector<std::string> _supportedInstanceExtensions;
	// Physical device (GPU) that Vulkan will use
	VkPhysicalDevice _physicalDevice;
	// Stores physical device properties (for e.g. checking device limits)
	VkPhysicalDeviceProperties _deviceProperties;
	// Stores the features available on the selected physical device (for e.g. checking if a feature is available)
	VkPhysicalDeviceFeatures _deviceFeatures;
	// Stores all available memory (type) properties for the physical device
	VkPhysicalDeviceMemoryProperties _deviceMemoryProperties;
	/** @brief Set of physical device features to be enabled for this example (must be set in the derived constructor) */
	VkPhysicalDeviceFeatures _enabledFeatures{};
	/** @brief Set of device extensions to be enabled for this example (must be set in the derived constructor) */
	std::vector<const char*> _enabledDeviceExtensions;
	std::vector<const char*> _enabledInstanceExtensions;
	/** @brief Optional pNext structure for passing extension structures to device creation */
	void* deviceCreatepNextChain = nullptr;
	/** @brief Logical device, application's view of the physical device (GPU) */
	VkDevice _device;
	// Handle to the device graphics queue that command buffers are submitted to
	VkQueue _presentQueue;
	// Depth buffer format (selected during Vulkan initialization)
	VkFormat depthFormat;
	// Command buffer pool
	VkCommandPool _cmdPool;
	/** @brief Pipeline stages used to wait at for graphics queue submissions */
	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// Contains command buffers and semaphores to be presented to the queue
	VkSubmitInfo submitInfo;
	// Command buffers used for rendering
	std::vector<VkCommandBuffer> _drawCmdBuffers;
	// Global render pass for frame buffer writes
	VkRenderPass _renderPass = VK_NULL_HANDLE;
	// List of available frame buffers (same as number of swap chain images)
	std::vector<VkFramebuffer> _frameBuffers;
	// Active frame buffer index
	uint32_t _currentBuffer = 0;
	// Descriptor set pool
	VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;
	// List of shader modules created (stored for cleanup)
	std::vector<VkShaderModule> shaderModules;
	// Pipeline cache object
	VkPipelineCache _pipelineCache;
	// Wraps the swap chain to present images (framebuffers) to the windowing system
	vk::VulkanSwapChain _swapChain;

	// Synchronization semaphores
	struct {
		// Swap chain image presentation
		VkSemaphore presentComplete;
		// Command buffer submission and execution
		VkSemaphore renderComplete;
	} _semaphores;

	std::vector<VkFence> _waitFences;

	std::shared_ptr<vk::VulkanDevice> _vulkanDevice;

// ----------------------------- vulkan rasterization helper -------------------
protected:

	void prepareFrame();

	void submitFrame();

	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);
};
};
#endif