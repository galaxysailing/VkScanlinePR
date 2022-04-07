#pragma once
#ifndef GALAXYSAILING_VK_DEBUG_H_
#define GALAXYSAILING_VK_DEBUG_H_

#include <vulkan/vulkan.h>

namespace Galaxysailing {
namespace vk {
    
namespace debug {
	void setupDebugging(
		VkInstance instance
		, VkDebugReportFlagsEXT flags
		, VkDebugReportCallbackEXT callBack);
}

}// end of namespace vk
}// end of namespace Galaxysailing

#endif