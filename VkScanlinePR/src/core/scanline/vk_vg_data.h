#pragma once
#ifndef GALAXYSAILING_VK_VG_DATA_H_
#define GALAXYSAILING_VK_VG_DATA_H_

#include "../vulkan/vulkan_buffer.h"
#include <glm/glm.hpp>
#include <memory>

#define NEW_VULKAN_BUFFER(T, device, usage, mem_prop, queue) (std::make_shared<vulkan::VulkanBuffer<T>>(device, usage, mem_prop, queue)); 

namespace Galaxysailing {
using namespace glm;

struct VkVGInputCurveData{
	// point data
	VULKAN_BUFFER_PTR(vec2) position;
	VULKAN_BUFFER_PTR(uint32_t) position_path_idx;

	// curve
	VULKAN_BUFFER_PTR(uint32_t) curve_position_map;
	VULKAN_BUFFER_PTR(uint8_t) curve_type;
	VULKAN_BUFFER_PTR(uint32_t) curve_path_idx;

	// numbers
	uint32_t n_curves;
	uint32_t n_points;

};

struct VkVGInputPathData {
	VULKAN_BUFFER_PTR(uint8_t) fill_rule;
	VULKAN_BUFFER_PTR(uint32_t) fill_info;

	uint32_t n_paths;
};

}
#endif