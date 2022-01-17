#pragma once
#ifndef GALAXYSAILING_VK_UTIL_H_
#define GALAXYSAILING_VK_UTIL_H_

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

#include <string>
#include <cstring>
#include <fstream>
#include <assert.h>
#include <sstream>
#include <stdexcept>
#include <iostream>

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << vk::util::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}



namespace Galaxysailing {

namespace vk{
    namespace util {

        /** @brief Returns an error code as a string */
        std::string errorString(VkResult errorCode);

        void exitFatal(const std::string& message, VkResult resultCode);
        void exitFatal(const std::string& message, int32_t exitCode);

        std::string read_file(const std::string filename);

        // Returns GLSL shader source text after preprocessing.
        std::string preprocess_shader(const std::string& source_name,
            shaderc_shader_kind kind,
            const std::string& source);

        // Compiles a shader to SPIR-V assembly. Returns the assembly text
        // as a string.
        std::string compile_file_to_assembly(const std::string& source_name,
            shaderc_shader_kind kind,
            const std::string& source,
            bool optimize);

        // Compiles a shader to a SPIR-V binary. Returns the binary as
        // a vector of 32-bit words.
        std::vector<uint32_t> compile_file(const std::string& source_name,
            shaderc_shader_kind kind,
            const std::string& source,
            bool optimize);
    }

}// end of namespace vk

}// end of namespace Galaxysailing

#endif
