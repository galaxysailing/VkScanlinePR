#include "vk_buffer.h"

namespace Galaxysailing {
namespace vk {

	VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset)
	{
		if (mapped)
		{
			vkUnmapMemory(device, memory);
			mapped = nullptr;
		}

		return vkMapMemory(device, memory, offset, size, 0, &mapped);
	}

	void Buffer::unmap()
	{
		if (mapped)
		{
			vkUnmapMemory(device, memory);
			mapped = nullptr;
		}
	}

	VkResult Buffer::bind(VkDeviceSize offset)
	{
		return vkBindBufferMemory(device, buffer, memory, offset);
	}

	/**
	* Setup the default descriptor for this buffer
	*
	* @param size (Optional) Size of the memory range of the descriptor
	* @param offset (Optional) Byte offset from beginning
	*
	*/
	void Buffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset)
	{
		descriptor.offset = offset;
		descriptor.buffer = buffer;
		descriptor.range = size;
	}

	void Buffer::copyTo(void* data, VkDeviceSize size)
	{
		assert(mapped);
		memcpy(mapped, data, size);
	}

	/**
	* Flush a memory range of the buffer to make it visible to the device
	*
	* @note Only required for non-coherent memory
	*
	* @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
	* @param offset (Optional) Byte offset from beginning
	*
	* @return VkResult of the flush call
	*/
	VkResult Buffer::flush(VkDeviceSize size, VkDeviceSize offset)
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
	}

	/**
	* Invalidate a memory range of the buffer to make it visible to the host
	*
	* @note Only required for non-coherent memory
	*
	* @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
	* @param offset (Optional) Byte offset from beginning
	*
	* @return VkResult of the invalidate call
	*/
	VkResult Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset)
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
	}

	void Buffer::destroy()
	{
		if (buffer)
		{
			vkDestroyBuffer(device, buffer, nullptr);
		}
		if (memory)
		{
			vkFreeMemory(device, memory, nullptr);
		}
	}

// ------------------------------ texel buffer ------------------------------------------
	VkResult TexelBuffer::createBufferView(VkFormat format, VkDeviceSize range) {
		VkBufferViewCreateInfo bufferViewCI{};
		bufferViewCI.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
		bufferViewCI.buffer = buffer;
		bufferViewCI.format = format;
		bufferViewCI.range = range;
		bufferViewCI.flags = 0;

		return vkCreateBufferView(device, &bufferViewCI, nullptr, &bufferView);
	}

};// end of namespace vk
};// end of namespace Galaxysailing