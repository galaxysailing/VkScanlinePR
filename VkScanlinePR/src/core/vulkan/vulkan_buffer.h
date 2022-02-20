#pragma once
#ifndef GALAXYSAILING_VULKAN_BUFFER_H_
#define GALAXYSAILING_VULKAN_BUFFER_H_

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

#include "../vk/vk_device.h"
#include "../vk/vk_initializer.h"

#define VULKAN_BUFFER_PTR(T) std::shared_ptr<vulkan::VulkanBuffer<T>>

namespace Galaxysailing {
namespace vulkan {

	template<class T>
	class VulkanBuffer {
	public:
		VulkanBuffer(std::shared_ptr<vk::VulkanDevice> device
			, VkBufferUsageFlags usage_flags
			, VkMemoryPropertyFlags memory_property_flags
			, VkQueue queue) {
			_device = device;
			_usage_flags = usage_flags;
			_memory_property_flags = memory_property_flags;
			_queue = queue;
		}

		void resizeWithoutCopy(uint32_t len) {
			VkDeviceSize new_size = static_cast<VkDeviceSize>(sizeof(T) * len);
			if (new_size == _size) {
				return;
			}

			if (_capacity < new_size) {
				destroy();
				_capacity = tableSizeFor(new_size);
				VK_CHECK_RESULT(_device->createBuffer(_usage_flags
					, _memory_property_flags
					, _capacity, &_buffer, &_memory));
			}
			_size = new_size;
			setupBufInfo();
		}

		void set(T& data, uint32_t len) {
			VkDeviceSize buf_size = static_cast<VkDeviceSize>(sizeof(T) * len);
			if (_buffer && _memory) {
				if (_capacity < buf_size) {
					throw std::runtime_error("Vulkan Buffer");
				}
				clear();
				updateBuffer(&data, _size);
				_size = buf_size;
			}
			else {

				// if '_buffer' is null or '_memory' is null,
				// then create new buffer
				destroy();
				if ((_memory_property_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) {
					createWithStagingCopy(&data, buf_size);
				}
				else if ((_memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
					createWithoutStagingCopy(&data, buf_size);
				}
			}
			setupBufInfo();
		}

		// set 
		void set(std::vector<T>& data) {
			VkDeviceSize buf_size = static_cast<VkDeviceSize>(sizeof(data[0]) * data.size());
			if (_buffer && _memory) {
				if (_capacity < buf_size) {
					throw std::runtime_error("Vulkan Buffer::set(vector) capacity too small.");
				}
				clear();
				updateBuffer(data.data(), buf_size);
				_size = buf_size;
			}
			else {
				// if '_buffer' is null or '_memory' is null,
				// then create new buffer
				destroy();
				if ((_memory_property_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) {
					createWithStagingCopy(data.data(), buf_size);
				}
				else if ((_memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
					createWithoutStagingCopy(data.data(), buf_size);
				}
			}
			setupBufInfo();
		}

		void clear() {
			VkDevice device = _device->logicalDevice;
			_host_data.ptr = nullptr;
			if (_host_data.buf) {
				vkDestroyBuffer(device, _host_data.buf, nullptr);
			}
			if (_host_data.memory) {
				//vkUnmapMemory(device, _host_data.memory);
				vkFreeMemory(device, _host_data.memory, nullptr);
			}
		}

		// release original memory
		void destroy() {
			VkDevice device = _device->logicalDevice;
			if (_host_data.ptr) {
				_host_data.ptr = nullptr;
				if (_host_data.buf) {
					vkDestroyBuffer(device, _host_data.buf, nullptr);
				}
				if (_host_data.memory) {
					vkFreeMemory(device, _host_data.memory, nullptr);
				}
			}
			if (_buffer) {
				vkDestroyBuffer(device, _buffer, nullptr);
			}
			if (_memory) {
				vkFreeMemory(device, _memory, nullptr);
			}
		}

		// ---------------------- for debug -----------------------
		T* cptr() {
			if (_host_data.ptr == nullptr) {
				VkCommandBuffer copy_cmd = _device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
				VkBufferCopy copy_region = {};
				copy_region.size = _size;
				VK_CHECK_RESULT(_device->createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT
					, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
					, _size, &_host_data.buf, &_host_data.memory));
				vkCmdCopyBuffer(copy_cmd, _buffer, _host_data.buf, 1, &copy_region);
				void* ppdata = (void*)_host_data.ptr;
				vkMapMemory(_device->logicalDevice, _host_data.memory, 0, _size, 0, &ppdata);
				_device->flushCommandBuffer(copy_cmd, _queue, true);
				_host_data.ptr = (T*)ppdata;
			}
			return _host_data.ptr;
		}
		void cptr_clear() {
			VkDevice device = _device->logicalDevice;
			if (_host_data.ptr) {
				_host_data.ptr = nullptr;
				if (_host_data.buf) {
					vkDestroyBuffer(device, _host_data.buf, nullptr);
				}
				if (_host_data.memory) {
					vkFreeMemory(device, _host_data.memory, nullptr);
				}
			}
		}
		// ----------------------------------------------------------

		T operator[](uint32_t index) {
			VkCommandBuffer copy_cmd = _device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			VkBufferCopy copy_region = {};
			copy_region.size = static_cast<VkDeviceSize>(sizeof(T));
			copy_region.srcOffset = static_cast<VkDeviceSize>(index * sizeof(T));

			VkBuffer buf = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;

			VK_CHECK_RESULT(_device->createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT
				, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				, copy_region.size, &buf, &memory));
			vkCmdCopyBuffer(copy_cmd, _buffer, buf, 1, &copy_region);
			void* pdata;
			vkMapMemory(_device->logicalDevice, memory, 0, copy_region.size, 0, &pdata);
			_device->flushCommandBuffer(copy_cmd, _queue, true);
			T res = *((T*)pdata);

			// destroy
			vkDestroyBuffer(_device->logicalDevice, buf, nullptr);
			vkFreeMemory(_device->logicalDevice, memory, nullptr);
			return res;
		}

		std::vector<T> get_list(uint32_t index, uint32_t size) {
			VkCommandBuffer copy_cmd = _device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			VkBufferCopy copy_region = {};
			copy_region.size = static_cast<VkDeviceSize>(sizeof(T) * size);
			copy_region.srcOffset = static_cast<VkDeviceSize>(index * sizeof(T));

			VkBuffer buf = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;

			VK_CHECK_RESULT(_device->createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT
				, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				, copy_region.size, &buf, &memory));
			vkCmdCopyBuffer(copy_cmd, _buffer, buf, 1, &copy_region);
			void* pdata;
			vkMapMemory(_device->logicalDevice, memory, 0, copy_region.size, 0, &pdata);
			_device->flushCommandBuffer(copy_cmd, _queue, true);
			std::vector<T> res;
			res.reverse(size);
			for (int i = 0; i < size; ++i) {
				res.push_back(((T*)pdata)[i]);
			}

			// destroy
			vkDestroyBuffer(_device->logicalDevice, buf, nullptr);
			vkFreeMemory(_device->logicalDevice, memory, nullptr);
			return res;
		}

		VkBuffer buffer() {
			return _buffer;
		}

		int size() {
			return _size / sizeof(T);
		}

		void setupBufferView(VkFormat format, VkDeviceSize range) {
			desc.buf_view = {};
			desc.buf_view.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
			desc.buf_view.buffer = _buffer;
			desc.buf_view.format = format;
			desc.buf_view.range = range;
			desc.buf_view.flags = 0;
		}


	public:
		struct {
			VkDescriptorBufferInfo buf_info;
			VkBufferViewCreateInfo buf_view;
		} desc;

	private:

		int tableSizeFor(int cap) {
			int n = cap - 1;
			n |= n >> 1;
			n |= n >> 2;
			n |= n >> 4;
			n |= n >> 8;
			n |= n >> 16;
			return (n < 0) ? 1 : (n >= MAXIMUM_CAPACITY) ? MAXIMUM_CAPACITY : n + 1;
		}

		void updateBuffer(T* data, VkDeviceSize buf_size) {
			VkDevice device = _device->logicalDevice;
			if (_memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT != 0) {
				void* cptr;
				vkMapMemory(device, _memory, 0, buf_size, 0, &cptr);
				memcpy(cptr, (void*)data, buf_size);
				vkUnmapMemory(device, _memory);
			}
		}

		void createWithoutStagingCopy(T* data, VkDeviceSize buf_size) {
			VkDevice device = _device->logicalDevice;
			_size = buf_size;
			_capacity = tableSizeFor(buf_size);

			VK_CHECK_RESULT(_device->createBuffer(_usage_flags
				, _memory_property_flags
				, _capacity, &_buffer, &_memory));

			void* cptr = nullptr;
			vkMapMemory(device, _memory, 0, _size, 0, &cptr);
			memcpy(cptr, (void*)data, (size_t)_size);
			vkUnmapMemory(device, _memory);
		}

		void createWithStagingCopy(T* data, VkDeviceSize buf_size) {
			VkDevice device = _device->logicalDevice;
			_size = buf_size;
			_capacity = tableSizeFor(buf_size);

			VkBuffer staging_buf;
			VkDeviceMemory staging_buf_mem;

			VK_CHECK_RESULT(_device->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT
				, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				, buf_size, &staging_buf, &staging_buf_mem));

			void* cptr = nullptr;
			vkMapMemory(device, staging_buf_mem, 0, buf_size, 0, &cptr);
			memcpy(cptr, (void*)data, (size_t)buf_size);
			vkUnmapMemory(device, staging_buf_mem);

			VK_CHECK_RESULT(_device->createBuffer(_usage_flags
				, _memory_property_flags
				, _capacity, &_buffer, &_memory));

			// copy from staging to device
			VkCommandBuffer copy_cmd = _device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			VkBufferCopy copy_region = {};
			copy_region.size = buf_size;
			vkCmdCopyBuffer(copy_cmd, staging_buf, _buffer, 1, &copy_region);
			_device->flushCommandBuffer(copy_cmd, _queue, true);
		}

		void setupBufInfo() {
			desc.buf_info.offset = 0;
			desc.buf_info.buffer = _buffer;
			desc.buf_info.range = _size;
		}

		VkBufferUsageFlags _usage_flags;
		VkMemoryPropertyFlags _memory_property_flags;
		struct {
			T* ptr = nullptr;
			VkBuffer buf = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
		} _host_data;
		std::shared_ptr<vk::VulkanDevice> _device;

		VkBuffer _buffer = VK_NULL_HANDLE;
		VkDeviceMemory _memory = VK_NULL_HANDLE;
		VkDeviceSize _size = 0;
		VkDeviceSize _capacity = 0;
		VkDeviceSize _alignment = 0;
		VkQueue _queue;

		static const int MAXIMUM_CAPACITY = 1 << 30;
	};
}// end of namespace vulkan
}// end of namespace Galaxysailing

#endif