#pragma once
#ifndef GALAXYSAILING_SCANLINE_RASTERIZER_H_
#define GALAXYSAILING_SCANLINE_RASTERIZER_H_

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <string>

#include "../vk/vk_device.h"
#include "../vk/vk_swapchain.h"
#include "../vk/vk_util.h"
#include "../vk/vk_debug.h"
#include "../vk/vk_vg_rasterizer.h"
#include "../rasterizer.h"

#include <GLFW/glfw3.h>
namespace Galaxysailing {

class ScanlineVGRasterizer : public VulkanVGRasterizerBase, public VGRasterizer {
// ----------------------------- rasterizer interface ------------------------
public:

    void initialize(void* window, uint32_t w, uint32_t h) override;

	void render() override;

    void loadVG(std::shared_ptr<VGContainer> vg) override;

    //void viewport(int x, int y, int w, int h) override;

    ~ScanlineVGRasterizer() {
        VK_CHECK_RESULT(vkDeviceWaitIdle(_device));
    }

// ------------------------------ Vulkan Base override ------------------------
private:
    
    void prepare() override;
    
    void getEnabledFeatures() override;

// ------------------------------ Scaline PR rasterization ---------------------
private:

    void mockDataload();

    void drawFrame();

    void prepareTexelBuffers();
    void setupDescriptorPool();
    void setupLayoutsAndDescriptors();
    void preparePipelines();
    void buildCommandBuffers();
    
private:

    std::shared_ptr<VGContainer> _vgContainer;
    std::vector<glm::ivec4> outputIndex;

    struct {
        vk::TexelBuffer outputIndexBuffer;

        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
        VkPipelineLayout pipelineLayout;

        struct {
            VkPipeline scanline;
        }pipelines;
        
    }graphics;


};
}
#endif