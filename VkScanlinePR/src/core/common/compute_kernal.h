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

    ComputeKernal(VkDevice device, VkPipelineCache ppl_cache
        , const vector<VkDescriptorType>& desc_types
        , VkCommandPool compute_cmd_pool
        , bool push_desc
        , const VkPipelineShaderStageCreateInfo &shader_stage_ci
        , PFN_vkCmdPushDescriptorSetKHR pfn_push_desc
        , vector<VkPushConstantRange>* push_const_range = nullptr)
        : _device(device),
        _pipeline_cache(ppl_cache),
        _vkCmdPushDescriptorSetKHR(pfn_push_desc) 
    {
        // layout binding
        vector<VkDescriptorSetLayoutBinding> ds_layout_bindings;
        for (size_t i = 0; i < desc_types.size(); ++i) {
            ds_layout_bindings.push_back(vk::initializer::descriptorSetLayoutBinding(desc_types[i], VK_SHADER_STAGE_COMPUTE_BIT, i));
        }
        VkDescriptorSetLayoutCreateInfo desc_layout_ci =
            vk::initializer::descriptorSetLayoutCreateInfo(ds_layout_bindings);

        _push_desc = push_desc;
        if (push_desc) {
            desc_layout_ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
        }
        else {
            // TODO
        }
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(_device, &desc_layout_ci, nullptr, &_desc_set_layout));

        // pipeline 
        VkPipelineLayoutCreateInfo ppl_layout_ci =
            vk::initializer::pipelineLayoutCreateInfo(&_desc_set_layout, 1);
        if (push_const_range != nullptr) {
            ppl_layout_ci.pPushConstantRanges = push_const_range->data();
            ppl_layout_ci.pushConstantRangeCount = push_const_range->size();
        }
        VK_CHECK_RESULT(vkCreatePipelineLayout(_device, &ppl_layout_ci, nullptr, &_pipeline_layout));

        VkComputePipelineCreateInfo compute_ppl_ci = vk::initializer::computePipelineCreateInfo(_pipeline_layout, 0);
        compute_ppl_ci.stage = shader_stage_ci;
        VK_CHECK_RESULT(vkCreateComputePipelines(_device, _pipeline_cache, 1, &compute_ppl_ci, nullptr, &_pipeline));

        // Create a command buffer for compute operations
        VkCommandBufferAllocateInfo cmd_buf_alloc_info =
            vk::initializer::commandBufferAllocateInfo(compute_cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
        VK_CHECK_RESULT(vkAllocateCommandBuffers(_device, &cmd_buf_alloc_info, &cmd_buffer));

        // GPU-GPU synchronization
        VkSemaphoreCreateInfo sem_ci= vk::initializer::semaphoreCreateInfo();
        VK_CHECK_RESULT(vkCreateSemaphore(_device, &sem_ci, nullptr, &semaphore));
    }

    // ---------------------- command ------------------------------
    ComputeKernal* beginCmdBuffer(bool one_time = false) {
        VkCommandBufferBeginInfo cmd_buf_info = vk::initializer::commandBufferBeginInfo();
        cmd_buf_info.flags = one_time ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        VK_CHECK_RESULT(vkBeginCommandBuffer(cmd_buffer, &cmd_buf_info));
        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
        return this;
    }

    ComputeKernal* cmdPushConst(uint32_t offset, uint32_t size, const void* pValues) {
        vkCmdPushConstants(cmd_buffer, _pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, offset, size, pValues);
        return this;
    }

    ComputeKernal* cmdPushDescSet(const vector<VkWriteDescriptorSet>& write_desc_sets) {
        _vkCmdPushDescriptorSetKHR(cmd_buffer
            , VK_PIPELINE_BIND_POINT_COMPUTE
            , _pipeline_layout
            , 0
            , static_cast<uint32_t>(write_desc_sets.size())
            , write_desc_sets.data());
        return this;
    }

    ComputeKernal* cmdDispatch(uint32_t groupX) {
        vkCmdDispatch(cmd_buffer, groupX, 1, 1);
        return this;
    }

    ComputeKernal* endCmdBuffer() {
        vkEndCommandBuffer(cmd_buffer);
        return this;
    }
    // -----------------------------------------------------------------

    void buildCmdBuffer(uint32_t groupX, const vector<VkWriteDescriptorSet>& write_desc_sets, bool one_time = false) {
        VkCommandBufferBeginInfo cmd_buf_info = vk::initializer::commandBufferBeginInfo();
        cmd_buf_info.flags = one_time? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT :VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        VK_CHECK_RESULT(vkBeginCommandBuffer(cmd_buffer, &cmd_buf_info));

        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
        if (_push_desc) {
            _vkCmdPushDescriptorSetKHR(cmd_buffer
                , VK_PIPELINE_BIND_POINT_COMPUTE
                , _pipeline_layout
                , 0
                , static_cast<uint32_t>(write_desc_sets.size())
                , write_desc_sets.data());
        }
        else {
            // TODO
        }

        vkCmdDispatch(cmd_buffer, groupX, 1, 1);

        vkEndCommandBuffer(cmd_buffer);
    }

    VkSubmitInfo submitInfo(bool is_first_draw, std::vector<VkSemaphore>& wait_sema, std::vector<VkSemaphore>& signal_sema, const VkPipelineStageFlags *wait_dst_stage_masks) {
        VkSubmitInfo sub = vk::initializer::submitInfo();
        
        if (!is_first_draw) {
            //wait_sema.push_back(semaphores.ready);
            sub.pWaitDstStageMask = wait_dst_stage_masks;
            sub.waitSemaphoreCount = static_cast<uint32_t>(wait_sema.size());
            sub.pWaitSemaphores = wait_sema.data();
        }
        signal_sema.push_back(semaphore);
        sub.signalSemaphoreCount = static_cast<uint32_t>(signal_sema.size());
        sub.pSignalSemaphores = signal_sema.data();
        sub.commandBufferCount = 1;
        sub.pCommandBuffers = &cmd_buffer;

        return sub;
    }

private:
    VkDevice _device = VK_NULL_HANDLE;
    VkPipelineCache _pipeline_cache = VK_NULL_HANDLE;
    PFN_vkCmdPushDescriptorSetKHR _vkCmdPushDescriptorSetKHR = VK_NULL_HANDLE;
    VkDescriptorSetLayout _desc_set_layout = VK_NULL_HANDLE;
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
    //VkDescriptorSet desc_set;
    VkPipeline _pipeline = VK_NULL_HANDLE;

    bool _push_desc = true;


public:
    VkCommandBuffer cmd_buffer = VK_NULL_HANDLE;
    VkSemaphore semaphore;
};

}

#endif