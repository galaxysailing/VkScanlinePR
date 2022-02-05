#pragma once
#ifndef GALAXYSAILING_SCANLINE_COMPUTE_H_
#define GALAXYSAILING_SCANLINE_COMPUTE_H_

#include <vulkan/vulkan.h>

#include <vector>
#include <string>

#include "../vk/vk_initializer.h"
#include "../vk/vk_util.h"

namespace Galaxysailing {

using namespace std;


class ComputeKernal {
public:
    //ComputeKernal() {}
    ComputeKernal(VkDevice device, VkPipelineCache pipeline_cache
        , vector<VkWriteDescriptorSet>& write_desc_sets
        , VkCommandPool compute_cmd_pool
        , bool push_desc
        , const VkPipelineShaderStageCreateInfo &shader_stage_ci,
        PFN_vkCmdPushDescriptorSetKHR pfn_push_desc) {
        _device = device;
        _pipeline_cache = pipeline_cache;
        _vkCmdPushDescriptorSetKHR = pfn_push_desc;
        _write_desc_sets.assign(write_desc_sets.begin(), write_desc_sets.end());

        vector<VkDescriptorSetLayoutBinding> ds_layout_bindings;
        for (size_t i = 0; i < _write_desc_sets.size(); ++i) {
            ds_layout_bindings.push_back(vk::initializer::descriptorSetLayoutBinding(_write_desc_sets[i].descriptorType, VK_SHADER_STAGE_COMPUTE_BIT, i));
        }
        VkDescriptorSetLayoutCreateInfo desc_layout_ci =
            vk::initializer::descriptorSetLayoutCreateInfo(ds_layout_bindings);

        if (push_desc) {
            desc_layout_ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
        }
        else {
            // TODO
        }

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(_device, &desc_layout_ci, nullptr, &_desc_set_layout));

        VkPipelineLayoutCreateInfo ppl_layout_ci =
            vk::initializer::pipelineLayoutCreateInfo(&_desc_set_layout, 1);
        VK_CHECK_RESULT(vkCreatePipelineLayout(_device, &ppl_layout_ci, nullptr, &_pipeline_layout));

        VkComputePipelineCreateInfo compute_ppl_ci = vk::initializer::computePipelineCreateInfo(_pipeline_layout, 0);
        compute_ppl_ci.stage = shader_stage_ci;
        VK_CHECK_RESULT(vkCreateComputePipelines(_device, _pipeline_cache, 1, &compute_ppl_ci, nullptr, &_pipeline));

        // Create a command buffer for compute operations
        VkCommandBufferAllocateInfo cmd_buf_alloc_info =
            vk::initializer::commandBufferAllocateInfo(compute_cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

        VK_CHECK_RESULT(vkAllocateCommandBuffers(_device, &cmd_buf_alloc_info, &cmd_buffer));

        VkSemaphoreCreateInfo sem_ci= vk::initializer::semaphoreCreateInfo();
        VK_CHECK_RESULT(vkCreateSemaphore(_device, &sem_ci, nullptr, &semaphores.ready));
        VK_CHECK_RESULT(vkCreateSemaphore(_device, &sem_ci, nullptr, &semaphores.complete));
    }

    void buildCmdBuffer(uint32_t x, int y) {
        VkCommandBufferBeginInfo cmd_buf_info = vk::initializer::commandBufferBeginInfo();
        cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        VK_CHECK_RESULT(vkBeginCommandBuffer(cmd_buffer, &cmd_buf_info));

        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);

        _vkCmdPushDescriptorSetKHR(cmd_buffer
            , VK_PIPELINE_BIND_POINT_COMPUTE
            , _pipeline_layout
            , 0
            , static_cast<uint32_t>(_write_desc_sets.size())
            , _write_desc_sets.data());

        vkCmdDispatch(cmd_buffer, x, y, 1);

        vkEndCommandBuffer(cmd_buffer);
    }


private:
    VkDevice _device;
    VkPipelineCache _pipeline_cache;
    PFN_vkCmdPushDescriptorSetKHR _vkCmdPushDescriptorSetKHR;
    VkDescriptorSetLayout _desc_set_layout;
    VkPipelineLayout _pipeline_layout;
    //VkDescriptorSet desc_set;
    VkPipeline _pipeline;

    vector<VkWriteDescriptorSet> _write_desc_sets;

public:
    VkCommandBuffer cmd_buffer;
    struct Semaphores {
        VkSemaphore ready{ 0L };
        VkSemaphore complete{ 0L };
    } semaphores;

};

}

#endif