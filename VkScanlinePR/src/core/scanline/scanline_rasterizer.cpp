#include "scanline_rasterizer.h"

#include <stdexcept>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <array>
#include <iostream>
#include <assert.h>
#include <algorithm>

#include <fstream>
#include <sstream>

#include <GLFW/glfw3.h>

#define GPU_VULKAN_BUFFER(T) NEW_VULKAN_BUFFER(T, _vulkanDevice, usage_flags, memory_property_flags, queue)

namespace Galaxysailing {

using _Base = Galaxysailing::VulkanVGRasterizerBase;
 
// -------------------------------- public interface --------------------------------
void ScanlineVGRasterizer::initialize(void* window, uint32_t w, uint32_t h)
{
    // about window
    _window = (GLFWwindow*)window;
    _width = w, _height = h;

    _Base::initVulkan();

    // Create a compute capable device queue
    vkGetDeviceQueue(_device, _vulkanDevice->queueFamilyIndices.compute, 0, &_compute.queue);
}

void ScanlineVGRasterizer::render()
{
    if (!_prepared) {
        prepare();
    }

    drawFrame();

}

void ScanlineVGRasterizer::loadVG(std::shared_ptr<VGContainer> vg_input)
{
    auto& vg = *vg_input;

    auto& point = vg.pointData;
    auto& curve = vg.curveData;
    auto& path = vg.pathData;
    
    int n_paths = path.pathIndex + 1;
    int n_curves = curve.curveIndex + 1;
    int n_points = point.pos.size();

    using std::vector;
    using namespace glm;

    // point 
    vector<vec2> position;
    vector<uint32_t> pos_path_idx;
    position.reserve(n_points);
    pos_path_idx.reserve(n_points);

    // curve
    vector<uint32_t> curve_pos_map;
    vector<uint8_t> curve_type;
    vector<uint32_t> curve_path_idx;
    curve_pos_map.reserve(n_curves + 1);
    curve_type.reserve(n_curves);
    curve_path_idx.reserve(n_curves);

    // path
    vector<uint8_t> path_fill_rule;
    vector<uint32_t> path_fill_info;
    path_fill_rule.reserve(n_paths);
    path_fill_info.reserve(n_paths);

    for (uint32_t pi = 0; pi < n_paths; ++pi) {
        uint32_t path_idx = pi;
        uint32_t curve_begin = path.curveIndices[pi];
        uint32_t curve_end = (pi != n_paths - 1 ? path.curveIndices[pi + 1] : n_curves);
        
        // process fill color
        glm::vec4& color = path.fillColor[pi];
        color.a *= path.fillOpacity[pi];
        color *= 255.0f;
        uint8_t col[4];
#pragma unroll
        for (int i = 0; i < 4; ++i) {
            col[i] = static_cast<uint8_t>(color[i]);
        }
        uint32_t c = *((uint32_t*)col);
        c = c & 0xFF000000 ? c : 0;
        path_fill_info.push_back(c);

        // process fill rule
        path_fill_rule.push_back(static_cast<uint8_t>(path.fillRule[pi]));

        for (uint32_t ci = curve_begin; ci < curve_end; ++ci) {
            uint32_t curve_idx = ci;
            uint32_t point_begin = curve.posIndices[ci];
            uint32_t point_end = (ci != n_curves - 1 ? curve.posIndices[ci + 1] : n_points);
            curve_path_idx.push_back(path_idx);
            curve_pos_map.push_back(point_begin);
            curve_type.push_back(static_cast<uint8_t>(curve.curveType[ci]));
            for (uint32_t poi = point_begin; poi < point_end; ++poi) {
                position.push_back(point.pos[poi]);
                pos_path_idx.push_back(path_idx);
            }
        }
    }

    // record final curve-pos map
    curve_pos_map.push_back(n_points - 1);

    auto& _in_curve = _compute.curve_input;
    auto& _in_path = _compute.path_input;

    VkQueue queue = _compute.queue;
    VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT 
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VkMemoryPropertyFlags memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    _in_curve.position = GPU_VULKAN_BUFFER(vec2);
    _in_curve.position_path_idx = GPU_VULKAN_BUFFER(uint32_t);

    _in_curve.curve_position_map = GPU_VULKAN_BUFFER(uint32_t);
    _in_curve.curve_type = GPU_VULKAN_BUFFER(uint8_t);
    _in_curve.curve_path_idx = GPU_VULKAN_BUFFER(uint32_t);

    _in_curve.n_curves = n_curves;
    _in_curve.n_points = n_points;

    _in_path.fill_info = GPU_VULKAN_BUFFER(uint32_t);
    _in_path.fill_rule = GPU_VULKAN_BUFFER(uint8_t);

    _in_curve.position->set(position);
    _in_curve.position_path_idx->set(pos_path_idx);

    _in_curve.curve_position_map->set(curve_pos_map);
    _in_curve.curve_type->set(curve_type);
    _in_curve.curve_path_idx->set(curve_path_idx);

    _in_path.n_paths = n_paths;
    _in_path.fill_info->set(path_fill_info);
    _in_path.fill_rule->set(path_fill_rule);

}

//void ScanlineVGRasterizer::viewport(int x, int y, int w, int h)
//{
//    if (!_initialized) {
//        throw std::runtime_error("ScanlineVGRasterizer is not initialized");
//        return;
//    }
//
//}

// ---------------------------------------------------------------------------------

// ------------------------------ Vulkan Base override -----------------------------

void ScanlineVGRasterizer::prepare()
{
    _Base::prepare();

    prepareGraphics();

    prepareComputeBuffers();
    prepareCompute();
    _prepared = true;
}

void ScanlineVGRasterizer::getEnabledFeatures()
{
    _enabledFeatures.wideLines = VK_TRUE;
}


// ------------------------------ Scaline PR rasterization -------------------------

void ScanlineVGRasterizer::prepareGraphics()
{
    /// test
    mockDataload();

    prepareTexelBuffers();
    setupDescriptorPool();
    setupLayoutsAndDescriptors();
    preparePipelines();
    buildCommandBuffers();
}
void ScanlineVGRasterizer::mockDataload() {
    std::ifstream fin;
    fin.open("test_data.csv");
    if (!fin.is_open()) {
        throw std::runtime_error("mock data file open failed");
    }
    auto replace_comma = [](std::string& str) {
        for (auto& c : str) { if (c == ',') { c = ' '; } }
    };
    std::string tstr;
    while (fin>>tstr) {
        replace_comma(tstr);
        glm::ivec4 v;
        std::istringstream iss(tstr);
        iss >> v.x >> v.y >> v.z >> v.w;
        //if (v.w == 0) {
        outputIndex.push_back(v);
        //}
    }
    std::cout << "load success\n";
}
void ScanlineVGRasterizer::drawFrame()
{
    static bool firstDraw = true;
    VkSubmitInfo computeSubmitInfo = vk::initializer::submitInfo();
    // FIXME find a better way to do this (without using fences, which is much slower)
    VkPipelineStageFlags computeWaitDstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    if (!firstDraw) {
        computeSubmitInfo.waitSemaphoreCount = 1;
        computeSubmitInfo.pWaitSemaphores = &_compute.semaphores.ready;
        computeSubmitInfo.pWaitDstStageMask = &computeWaitDstStageMask;
    }
    else {
        firstDraw = false;
    }
    computeSubmitInfo.signalSemaphoreCount = 1;
    computeSubmitInfo.pSignalSemaphores = &_compute.semaphores.complete;
    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &_compute.commandBuffer;

    VK_CHECK_RESULT(vkQueueSubmit(_compute.queue, 1, &computeSubmitInfo, VK_NULL_HANDLE));

    // Submit graphics commands
    _Base::prepareFrame();
    std::array<VkPipelineStageFlags,2> waitDstStageMask = {
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    std::array<VkSemaphore,2> waitSemaphores = {
        _semaphores.presentComplete, _compute.semaphores.complete
    };
    std::array<VkSemaphore,2> signalSemaphores = {
        _semaphores.renderComplete, _compute.semaphores.ready
    };
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitDstStageMask.data();

    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_drawCmdBuffers[_currentBuffer];
    VK_CHECK_RESULT(vkQueueSubmit(_presentQueue, 1, &submitInfo, VK_NULL_HANDLE));

    _Base::submitFrame();

    //// for debug
    //int32_t* ptr = _compute.storage_buffers.path_visible->cptr();
    //printf("-------------------- begin --------------------\n");
    //for (int i = 0; i < _compute.curve_input.n_points; ++i) {
    //    printf("0x%x\n", *ptr);
    //}
    //printf("-------------------- end --------------------\n");
}

void ScanlineVGRasterizer::prepareTexelBuffers()
{
	// Vertex shader uniform buffer block
    _vulkanDevice->createBuffer(
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &graphics.outputIndexBuffer,
        sizeof(outputIndex[0]) * outputIndex.size());
	VK_CHECK_RESULT(graphics.outputIndexBuffer.map());
    VK_CHECK_RESULT(graphics.outputIndexBuffer.createBufferView(VK_FORMAT_R32G32B32A32_SINT, VK_WHOLE_SIZE));

    graphics.outputIndexBuffer.copyTo(outputIndex.data(), sizeof(outputIndex[0]) * outputIndex.size());
}

void ScanlineVGRasterizer::setupDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes = {
        // test
        vk::initializer::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1),
        ///
        vk::initializer::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4),
        vk::initializer::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
    };

    VkDescriptorPoolCreateInfo descriptorPoolInfo =
        vk::initializer::descriptorPoolCreateInfo(poolSizes, 3);

    VK_CHECK_RESULT(vkCreateDescriptorPool(_device, &descriptorPoolInfo, nullptr, &_descriptorPool));
}

void ScanlineVGRasterizer::setupLayoutsAndDescriptors()
{
    // graphics
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        vk::initializer::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
    };
    VkDescriptorSetLayoutCreateInfo descriptorLayout =
        vk::initializer::descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(_device, &descriptorLayout, nullptr, &graphics.descriptorSetLayout));

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
        vk::initializer::pipelineLayoutCreateInfo(&graphics.descriptorSetLayout, 1);
    VK_CHECK_RESULT(vkCreatePipelineLayout(_device, &pipelineLayoutCreateInfo, nullptr, &graphics.pipelineLayout));

    // Set
    VkDescriptorSetAllocateInfo allocInfo =
        vk::initializer::descriptorSetAllocateInfo(_descriptorPool, &graphics.descriptorSetLayout, 1);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(_device, &allocInfo, &graphics.descriptorSet));

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        vk::initializer::writeDescriptorSet(graphics.descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 0, &graphics.outputIndexBuffer.bufferView)
    };
    vkUpdateDescriptorSets(_device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);


    // compute
    
}

void ScanlineVGRasterizer::preparePipelines()
{
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
        vk::initializer::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, 0, VK_FALSE);

    VkPipelineRasterizationStateCreateInfo rasterizationState =
        vk::initializer::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL
            , VK_CULL_MODE_NONE
            , VK_FRONT_FACE_COUNTER_CLOCKWISE
            , 2.0f
            , 0);

    VkPipelineColorBlendAttachmentState blendAttachmentState =
        vk::initializer::pipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
            , VK_FALSE);

    VkPipelineColorBlendStateCreateInfo colorBlendState =
        vk::initializer::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

    VkPipelineDepthStencilStateCreateInfo depthStencilState =
        vk::initializer::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);

    VkPipelineViewportStateCreateInfo viewportState =
        vk::initializer::pipelineViewportStateCreateInfo(1, 1, 0);

    VkPipelineMultisampleStateCreateInfo multisampleState =
        vk::initializer::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);

    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState =
        vk::initializer::pipelineDynamicStateCreateInfo(dynamicStateEnables, 0);

    // Rendering pipeline
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    shaderStages[0] = loadShader("shaders/scanlinepr.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader("shaders/scanlinepr.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = vk::initializer::pipelineCreateInfo(graphics.pipelineLayout, _renderPass);


    //// Assign to vertex buffer
    VkPipelineVertexInputStateCreateInfo inputState = vk::initializer::pipelineVertexInputStateCreateInfo();
    //inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(inputBindings.size());
    //inputState.pVertexBindingDescriptions = inputBindings.data();
    //inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(inputAttributes.size());
    //inputState.pVertexAttributeDescriptions = inputAttributes.data();

    pipelineCreateInfo.pVertexInputState = &inputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.renderPass = _renderPass;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(_device, _pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphics.pipelines.scanline));

    

}

void ScanlineVGRasterizer::prepareCompute()
{
// --------------------------------------------------------- compute function ------------------------------------------------------------
    // Create compute pipeline
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        vk::initializer::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0),
        vk::initializer::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1),
        vk::initializer::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 2),
        vk::initializer::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 3),
        vk::initializer::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 4),
    };

    VkDescriptorSetLayoutCreateInfo descriptorLayout =
        vk::initializer::descriptorSetLayoutCreateInfo(setLayoutBindings);

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(_device, &descriptorLayout, nullptr, &_compute.descriptorSetLayout));

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
        vk::initializer::pipelineLayoutCreateInfo(&_compute.descriptorSetLayout, 1);

    VK_CHECK_RESULT(vkCreatePipelineLayout(_device, &pipelineLayoutCreateInfo, nullptr, &_compute.pipelineLayout));

    VkDescriptorSetAllocateInfo allocInfo =
        vk::initializer::descriptorSetAllocateInfo(_descriptorPool, &_compute.descriptorSetLayout, 1);

    VK_CHECK_RESULT(vkAllocateDescriptorSets(_device, &allocInfo, &_compute.descriptorSet));

    auto& _c = _compute;
    auto& _cin_curve = _c.curve_input;
    auto& _csb = _c.storage_buffers;
    auto& _cub = _c.uniform_buffers;
    std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets = {
            vk::initializer::writeDescriptorSet(_compute.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &_cin_curve.position->descriptor.buf_info),
            vk::initializer::writeDescriptorSet(_compute.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &_cin_curve.position_path_idx->descriptor.buf_info),
            vk::initializer::writeDescriptorSet(_compute.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, &_csb.transformed_pos->descriptor.buf_info),
            vk::initializer::writeDescriptorSet(_compute.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, &_csb.path_visible->descriptor.buf_info),
            vk::initializer::writeDescriptorSet(_compute.descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, &_cub.transformPosUBO->descriptor.buf_info)
    };
    vkUpdateDescriptorSets(_device, static_cast<uint32_t>(computeWriteDescriptorSets.size()), computeWriteDescriptorSets.data(), 0, nullptr);

    VkComputePipelineCreateInfo computePipelineCreateInfo = vk::initializer::computePipelineCreateInfo(_compute.pipelineLayout, 0);
    computePipelineCreateInfo.stage = loadShader("shaders/compute/transformPos.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
    VK_CHECK_RESULT(vkCreateComputePipelines(_device, _pipelineCache, 1, &computePipelineCreateInfo, nullptr, &_compute.pipeline));

// ---------------------------------------------------------------------------------------------------------------------------------------

    // Separate command pool as queue family for compute may be different than graphics
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = _vulkanDevice->queueFamilyIndices.compute;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(_device, &cmdPoolInfo, nullptr, &_compute.commandPool));

    // Create a command buffer for compute operations
    VkCommandBufferAllocateInfo cmdBufAllocateInfo =
        vk::initializer::commandBufferAllocateInfo(_compute.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

    VK_CHECK_RESULT(vkAllocateCommandBuffers(_device, &cmdBufAllocateInfo, &_compute.commandBuffer));

    // Semaphores for graphics / compute synchronization
    VkSemaphoreCreateInfo semaphoreCreateInfo = vk::initializer::semaphoreCreateInfo();
    VK_CHECK_RESULT(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_compute.semaphores.ready));
    VK_CHECK_RESULT(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_compute.semaphores.complete));

    // Build a single command buffer containing the compute dispatch commands
    buildComputeCommandBuffer();

}

void ScanlineVGRasterizer::buildComputeCommandBuffer()
{
    VkCommandBufferBeginInfo cmdBufInfo = vk::initializer::commandBufferBeginInfo();
    cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VK_CHECK_RESULT(vkBeginCommandBuffer(_compute.commandBuffer, &cmdBufInfo));

    // Acquire the storage buffers from the graphics queue
    //addGraphicsToComputeBarriers(_compute.commandBuffer);

    vkCmdBindPipeline(_compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _compute.pipeline);

    vkCmdBindDescriptorSets(_compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _compute.pipelineLayout, 0, 1, &_compute.descriptorSet, 0, 0);

    // inline int divup(int a, int b) { return (a + (b - 1)) / b; }
    vkCmdDispatch(_compute.commandBuffer, 256, (_compute.curve_input.n_points + 255) / 256, 1);

    // release the storage buffers back to the graphics queue
    //addComputeToGraphicsBarriers(compute.commandBuffers[i]);
    vkEndCommandBuffer(_compute.commandBuffer);
}

void ScanlineVGRasterizer::addComputeToComputeBarriers(VkCommandBuffer commandBuffer)
{
    VkBufferMemoryBarrier bufferBarrier = vk::initializer::bufferMemoryBarrier();
    bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    bufferBarrier.srcQueueFamilyIndex = _vulkanDevice->queueFamilyIndices.compute;
    bufferBarrier.dstQueueFamilyIndex = _vulkanDevice->queueFamilyIndices.compute;
    bufferBarrier.size = VK_WHOLE_SIZE;
    std::vector<VkBufferMemoryBarrier> bufferBarriers;
    bufferBarriers.push_back(bufferBarrier);
    bufferBarrier.buffer = _compute.storage_buffers.transformed_pos->buffer();
    bufferBarriers.push_back(bufferBarrier);
    bufferBarrier.buffer = _compute.storage_buffers.path_visible->buffer();
    bufferBarriers.push_back(bufferBarrier);
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_FLAGS_NONE,
        0, nullptr,
        static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(),
        0, nullptr);
}

void ScanlineVGRasterizer::buildCommandBuffers()
{
    VkCommandBufferBeginInfo cmdBufInfo = vk::initializer::commandBufferBeginInfo();
    VkClearValue clearValue = { {{1.0f, 1.0f, 1.0f, 1.0f}} };

    VkRenderPassBeginInfo renderPassBeginInfo = vk::initializer::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = _renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = _width;
    renderPassBeginInfo.renderArea.extent.height = _height;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;

    for (int32_t i = 0; i < _drawCmdBuffers.size(); ++i)
    {
        // Set target frame buffer
        renderPassBeginInfo.framebuffer = _frameBuffers[i];

        VK_CHECK_RESULT(vkBeginCommandBuffer(_drawCmdBuffers[i], &cmdBufInfo));

        // Acquire storage buffers from compute queue
        //addComputeToGraphicsBarriers(drawCmdBuffers[i]);

        // Draw the particle system using the update vertex buffer

        vkCmdBeginRenderPass(_drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = vk::initializer::viewport((float)_width, (float)_height, 0.0f, 1.0f);
        vkCmdSetViewport(_drawCmdBuffers[i], 0, 1, &viewport);
        
        VkRect2D scissor = vk::initializer::rect2D(_width, _height, 0, 0);
        vkCmdSetScissor(_drawCmdBuffers[i], 0, 1, &scissor);

        // Render path
        vkCmdBindPipeline(_drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipelines.scanline);
        vkCmdBindDescriptorSets(_drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipelineLayout, 0, 1, &graphics.descriptorSet, 0, NULL);
        vkCmdDraw(_drawCmdBuffers[i], outputIndex.size() * 2, 1, 0, 0);

        //drawUI(drawCmdBuffers[i]);

        vkCmdEndRenderPass(_drawCmdBuffers[i]);

        // release the storage buffers to the compute queue
        //addGraphicsToComputeBarriers(drawCmdBuffers[i]);

        VK_CHECK_RESULT(vkEndCommandBuffer(_drawCmdBuffers[i]));
    }
}

void ScanlineVGRasterizer::prepareComputeBuffers()
{
    auto& _c = _compute;
    auto& _csb = _c.storage_buffers;
    auto& _cub = _c.uniform_buffers;
    auto& _in_curve = _c.curve_input;
    auto& _in_path = _c.path_input;
    
    VkQueue queue = _c.queue;
    VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VkMemoryPropertyFlags memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    // storage buffer
    _csb.transformed_pos = GPU_VULKAN_BUFFER(vec2);
    _csb.path_visible = GPU_VULKAN_BUFFER(int32_t);

    _csb.transformed_pos->resizeWithoutCopy(_in_curve.n_points * sizeof(vec2));
    _csb.path_visible->resizeWithoutCopy(_in_path.n_paths * sizeof(int32_t));


    // uniform buffer
    usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 
        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    _cub.transformPosUBO = GPU_VULKAN_BUFFER(UBO);
    
    _c.ubo.n_points = _in_curve.n_points;
    _c.ubo.w = _width;
    _c.ubo.h = _height;
    _c.ubo.m0 = vec4(1, 0, 0, 0);
    _c.ubo.m1 = vec4(0, 1, 0, 0);
    _c.ubo.m2 = vec4(0, 0, 1, 0);
    _c.ubo.m3 = vec4(0, 0, 0, 1);

    _cub.transformPosUBO->set(_c.ubo, sizeof(UBO));
    
}

}