#pragma once
#ifndef GALAXYSAILING_VK_SWAPCHAIN_H_
#define GALAXYSAILING_VK_SWAPCHAIN_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <vector>

#include "vk_util.h"

namespace Galaxysailing {

namespace vk {
typedef struct _SwapChainBuffers {
	VkImage image;
	VkImageView view;
} SwapChainBuffer;

class VulkanSwapChain
{
private:
	VkInstance _instance;
	VkDevice _device;
	VkPhysicalDevice _physicalDevice;
	VkSurfaceKHR _surface;
	// Function pointers
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR _fpGetPhysicalDeviceSurfaceSupportKHR;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR _fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR _fpGetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR _fpGetPhysicalDeviceSurfacePresentModesKHR;
	PFN_vkCreateSwapchainKHR _fpCreateSwapchainKHR;
	PFN_vkDestroySwapchainKHR _fpDestroySwapchainKHR;
	PFN_vkGetSwapchainImagesKHR _fpGetSwapchainImagesKHR;
	PFN_vkAcquireNextImageKHR _fpAcquireNextImageKHR;
	PFN_vkQueuePresentKHR _fpQueuePresentKHR;
public:
	VkFormat colorFormat;
	VkColorSpaceKHR colorSpace;
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	uint32_t imageCount;
	std::vector<VkImage> images;
	std::vector<SwapChainBuffer> buffers;
	uint32_t queueNodeIndex = UINT32_MAX;

	void initSurface(GLFWwindow* window);

	void connect(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
	void create(uint32_t* width, uint32_t* height, bool vsync = false);
	VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex);
	VkResult queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE);
	void cleanup();
};

}// end of namespace vk
}// end of namespace Galaxysailing

#endif