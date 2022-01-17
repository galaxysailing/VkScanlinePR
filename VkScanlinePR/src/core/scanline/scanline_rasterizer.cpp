#include "scanline_rasterizer.h"

#include <stdexcept>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <array>
#include <iostream>
#include <assert.h>

#include <fstream>
#include <sstream>

#include <GLFW/glfw3.h>


namespace Galaxysailing {

using _Base = Galaxysailing::VulkanVGRasterizerBase;

// -------------------------------- public interface --------------------------------
void ScanlineVGRasterizer::initialize(void* window, uint32_t w, uint32_t h)
{
    // about window
    _window = (GLFWwindow*)window;
    _width = w, _height = h;

    _Base::initVulkan();
    prepare();
}

void ScanlineVGRasterizer::render()
{
    if (!_prepared) {
        throw std::runtime_error("ScanlineVGRasterizer is not initialized");
    }

    drawFrame();

}

void ScanlineVGRasterizer::loadVG(std::shared_ptr<VGContainer> vg)
{
    _vgContainer = vg;
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
    /// test
    mockDataload();

    prepareTexelBuffers();
    setupDescriptorPool();
    setupLayoutsAndDescriptors();
    preparePipelines();
    buildCommandBuffers();
    _prepared = true;
}

void ScanlineVGRasterizer::getEnabledFeatures()
{
    _enabledFeatures.wideLines = VK_TRUE;
}

// ------------------------------ Scaline PR rasterization -------------------------
void ScanlineVGRasterizer::mockDataload() {
    std::ifstream fin;
    fin.open("test_data2.csv");
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
    // Submit graphics commands
    _Base::prepareFrame();
    VkPipelineStageFlags waitDstStageMask[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSemaphore waitSemaphores[1] = {
        _semaphores.presentComplete
    };
    VkSemaphore signalSemaphores[1] = {
        _semaphores.renderComplete
    };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitDstStageMask;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_drawCmdBuffers[_currentBuffer];
    VK_CHECK_RESULT(vkQueueSubmit(_presentQueue, 1, &submitInfo, VK_NULL_HANDLE));

    _Base::submitFrame();


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
        vk::initializer::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 3)
    };

    VkDescriptorPoolCreateInfo descriptorPoolInfo =
        vk::initializer::descriptorPoolCreateInfo(poolSizes, 3);

    VK_CHECK_RESULT(vkCreateDescriptorPool(_device, &descriptorPoolInfo, nullptr, &_descriptorPool));
}

void ScanlineVGRasterizer::setupLayoutsAndDescriptors()
{
    // Set layout
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        vk::initializer::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
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

}