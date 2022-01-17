#include "vk_vg_rasterizer.h"

#include <stdexcept>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <array>
#include <iostream>
#include <assert.h>

#include <GLFW/glfw3.h>

namespace Galaxysailing {
// ----------------------------- private inner function ----------------------------
void VulkanVGRasterizerBase::initVulkan()
{
	//// to initialize the Vulkan library
	bool err;
	err = createInstance();
	if (err) {
		throw std::runtime_error("failed to create instance!");
	}

	if (settings.validation)
	{
		VkDebugReportFlagsEXT debugReportFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		vk::debug::setupDebugging(_instance, debugReportFlags, VK_NULL_HANDLE);
	}

	// Physical device
	uint32_t gpuCount = 0;
	// Get number of available physical devices
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices(_instance, &gpuCount, nullptr));
	if (gpuCount == 0) {
		throw std::runtime_error("No device with Vulkan support found");
	}
	// Enumerate devices
	std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices(_instance, &gpuCount, physicalDevices.data()));


	// GPU selection
	uint32_t selectedDevice = 0;

	_physicalDevice = physicalDevices[selectedDevice];
	// Store properties (including limits), features and memory properties of the physical device (so that examples can check against them)
	vkGetPhysicalDeviceProperties(_physicalDevice, &_deviceProperties);
	vkGetPhysicalDeviceFeatures(_physicalDevice, &_deviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &_deviceMemoryProperties);

	//// Derived examples can override this to set actual features (based on above readings) to enable for logical device creation
	//getEnabledFeatures();

	// Vulkan device creation
	// This is handled by a separate class that gets a logical device representation
	// and encapsulates functions related to a device
	_vulkanDevice = std::make_shared<vk::VulkanDevice>(_physicalDevice);
	VkResult res = _vulkanDevice->createLogicalDevice(enabledFeatures, enabledDeviceExtensions, deviceCreatepNextChain);
	/*if (res != VK_SUCCESS) {
		vk::util::exitFatal("Could not create Vulkan device: \n" + vk::util::errorString(res), res);
		return false;
	}*/
	_device = _vulkanDevice->logicalDevice;

	// Get a graphics queue from the device
	vkGetDeviceQueue(_device, _vulkanDevice->queueFamilyIndices.graphics, 0, &_presentQueue);

	//// Find a suitable depth format
	//VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(physicalDevice, &depthFormat);
	//assert(validDepthFormat);

	_swapChain.connect(_instance, _physicalDevice, _device);

	// Create synchronization objects
	VkSemaphoreCreateInfo semaphoreCreateInfo = vk::initializer::semaphoreCreateInfo();
	// Create a semaphore used to synchronize image presentation
	// Ensures that the image is displayed before we start submitting new commands to the queue
	VK_CHECK_RESULT(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_semaphores.presentComplete));
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands have been submitted and executed
	VK_CHECK_RESULT(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_semaphores.renderComplete));

	// Set up submit info structure
	// Semaphores will stay the same during application lifetime
	// Command buffer submission info is set by each example
	submitInfo = vk::initializer::submitInfo();
	submitInfo.pWaitDstStageMask = &submitPipelineStages;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &_semaphores.presentComplete;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &_semaphores.renderComplete;

}

void VulkanVGRasterizerBase::prepare()
{
	//if (vulkanDevice->enableDebugMarkers) {
	//    vks::debugmarker::setup(device);
	//}

	initSwapchain();
	createCommandPool();
	setupSwapChain();
	createCommandBuffers();
	createSynchronizationPrimitives();
	//setupDepthStencil();
	setupRenderPass();
	createPipelineCache();
	setupFrameBuffer();

	//settings.overlay = settings.overlay && (!benchmark.active);
	//if (settings.overlay) {
	//    UIOverlay.device = vulkanDevice;
	//    UIOverlay.queue = queue;
	//    UIOverlay.shaders = {
	//        loadShader(getShadersPath() + "base/uioverlay.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
	//        loadShader(getShadersPath() + "base/uioverlay.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
	//    };
	//    UIOverlay.prepareResources();
	//    UIOverlay.preparePipeline(pipelineCache, renderPass);
	//}

}

// ----------------------------- vulkan builder function ---------------------------

bool VulkanVGRasterizerBase::createInstance()
{
	//if (enableValidationLayers && !checkValidationLayerSupport()) {
	//    throw std::runtime_error("validation layers requested, but not available!");
	//}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Scanline Path Rendering";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.pEngineName = "Scanline Path Rendering";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.apiVersion = _apiVersion;

	int supported = glfwVulkanSupported();
	if (supported == GLFW_TRUE) {
		// do nothing
	}
	else {
		std::cout << "vulkan is minimally available\n";
	}

	// If **successful**,the list will always contain `VK_KHR_surface`
	// windows: VK_KHR_win32_surface
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> instanceExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	// Get extensions supported by the instance and store for later use
	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	if (extCount > 0)
	{
		std::vector<VkExtensionProperties> extensions(extCount);
		if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
		{
			for (VkExtensionProperties extension : extensions)
			{
				_supportedInstanceExtensions.push_back(extension.extensionName);
			}
		}
	}

	// Enabled requested instance extensions
	if (_enabledInstanceExtensions.size() > 0)
	{
		for (const char* enabledExtension : _enabledInstanceExtensions)
		{
			// Output message if requested extension is not available
			if (std::find(_supportedInstanceExtensions.begin(), _supportedInstanceExtensions.end(), enabledExtension) == _supportedInstanceExtensions.end())
			{
				throw std::runtime_error("Enabled instance extension \"" + std::string(enabledExtension) + "\" is not present at instance level\n");
				//std::cerr << "Enabled instance extension \"" << enabledExtension << "\" is not present at instance level\n";
			}
			instanceExtensions.push_back(enabledExtension);
		}
	}

	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.pApplicationInfo = &appInfo;

	if (instanceExtensions.size() > 0)
	{
		if (settings.validation)
		{
			instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
	}

	// The VK_LAYER_KHRONOS_validation contains all current validation functionality.
	// Note that on Android this layer requires at least NDK r20
	const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
	if (settings.validation)
	{
		// Check if this layer is available at instance level
		uint32_t instanceLayerCount;
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
		std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
		bool validationLayerPresent = false;
		for (VkLayerProperties layer : instanceLayerProperties) {
			if (strcmp(layer.layerName, validationLayerName) == 0) {
				validationLayerPresent = true;
				break;
			}
		}
		if (validationLayerPresent) {
			instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
			instanceCreateInfo.enabledLayerCount = 1;
		}
		else {
			throw std::runtime_error("Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled");
		}
	}


	return vkCreateInstance(&instanceCreateInfo, nullptr, &_instance) != VK_SUCCESS;
}

void VulkanVGRasterizerBase::initSwapchain()
{
	_swapChain.initSurface(_window);
}

void VulkanVGRasterizerBase::createCommandPool()
{

	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = _swapChain.queueNodeIndex;
	// `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`
	// Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VK_CHECK_RESULT(vkCreateCommandPool(_device, &cmdPoolInfo, nullptr, &_cmdPool));

}

void VulkanVGRasterizerBase::setupSwapChain()
{
	_swapChain.create(&_width, &_height, settings.vsync);
}

void VulkanVGRasterizerBase::createCommandBuffers()
{
	// Create one command buffer for each swap chain image and reuse for rendering
	_drawCmdBuffers.resize(_swapChain.imageCount);

	VkCommandBufferAllocateInfo cmdBufAllocateInfo =
		vk::initializer::commandBufferAllocateInfo(
			_cmdPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			static_cast<uint32_t>(_drawCmdBuffers.size()));

	VK_CHECK_RESULT(vkAllocateCommandBuffers(_device, &cmdBufAllocateInfo, _drawCmdBuffers.data()));
}

void VulkanVGRasterizerBase::createSynchronizationPrimitives()
{
	// Wait fences to sync command buffer access
	VkFenceCreateInfo fenceCreateInfo = vk::initializer::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	_waitFences.resize(_drawCmdBuffers.size());
	for (auto& fence : _waitFences) {
		VK_CHECK_RESULT(vkCreateFence(_device, &fenceCreateInfo, nullptr, &fence));
	}
}

void VulkanVGRasterizerBase::setupRenderPass()
{
	std::array<VkAttachmentDescription, 1> attachments = {};
	// Color attachment
	attachments[0].format = _swapChain.colorFormat;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// Depth attachment
	//attachments[1].format = depthFormat;
	//attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	//attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	//attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	//attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	//attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	//attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	//attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//VkAttachmentReference depthReference = {};
	//depthReference.attachment = 1;
	//depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	//subpassDescription.pDepthStencilAttachment = &depthReference;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;
	subpassDescription.pResolveAttachments = nullptr;

	// Subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	VK_CHECK_RESULT(vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_renderPass));
}

void VulkanVGRasterizerBase::createPipelineCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	VK_CHECK_RESULT(vkCreatePipelineCache(_device, &pipelineCacheCreateInfo, nullptr, &_pipelineCache));
}

void VulkanVGRasterizerBase::setupFrameBuffer()
{
	std::array<VkImageView, 1> attachments{};

	// Depth/Stencil attachment is the same for all frame buffers
	//attachments[1] = depthStencil.view;

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = NULL;
	frameBufferCreateInfo.renderPass = _renderPass;
	frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	frameBufferCreateInfo.pAttachments = attachments.data();
	frameBufferCreateInfo.width = _width;
	frameBufferCreateInfo.height = _height;
	frameBufferCreateInfo.layers = 1;

	// Create frame buffers for every swap chain image
	_frameBuffers.resize(_swapChain.imageCount);
	for (uint32_t i = 0; i < _frameBuffers.size(); i++)
	{
		attachments[0] = _swapChain.buffers[i].view;
		VK_CHECK_RESULT(vkCreateFramebuffer(_device, &frameBufferCreateInfo, nullptr, &_frameBuffers[i]));
	}
}

void VulkanVGRasterizerBase::windowResize()
{
	if (!_prepared)
	{
		return;
	}
	//_prepared = false;
	////----------------------Handling minimization----------------------------
	//int width = 0, height = 0;
	//glfwGetFramebufferSize(_window, &width, &height);
	//while (width == 0 || height == 0) {
	//	glfwGetFramebufferSize(_window, &width, &height);
	//	glfwWaitEvents();
	//}
	//_width = static_cast<uint32_t>(width);
	//_height = static_cast<uint32_t>(height);
	//vkDeviceWaitIdle(_device);


}

// ----------------------------- vulkan rasterization helper -----------------------------
void VulkanVGRasterizerBase::prepareFrame()
{
	// Acquire the next image from the swap chain
	VkResult result = _swapChain.acquireNextImage(_semaphores.presentComplete, &_currentBuffer);
	// Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
	if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
		//windowResize();
	}
	else {
		VK_CHECK_RESULT(result);
	}
}
void VulkanVGRasterizerBase::submitFrame()
{
	VkResult result = _swapChain.queuePresent(_presentQueue, _currentBuffer, _semaphores.renderComplete);
	if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR))) {
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			// Swap chain is no longer compatible with the surface and needs to be recreated
			//windowResize();
			return;
		}
		else {
			VK_CHECK_RESULT(result);
		}
	}
	VK_CHECK_RESULT(vkQueueWaitIdle(_presentQueue));
}
};