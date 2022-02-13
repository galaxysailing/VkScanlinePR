#pragma once
#ifndef GALAXYSAILING_SCANLINE_RASTERIZER_H_
#define GALAXYSAILING_SCANLINE_RASTERIZER_H_

#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <optional>
#include <string>

#include "../vk/vk_device.h"
#include "../vk/vk_swapchain.h"
#include "../vk/vk_util.h"
#include "../vk/vk_debug.h"
#include "../vulkan/vk_vg_rasterizer.h"
#include "../vulkan/vulkan_buffer.h"
#include "../rasterizer.h"

#include "../common/compute_kernal.h"
#include "compute_ubo.h"
#include "vk_vg_data.h"

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
using namespace glm;

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
    void prepareGraphics();

    void prepareCompute();

private:

    void mockDataload();

    void drawFrame();

    void prepareTexelBuffers();
    void buildCommandBuffers();

    void setupDescriptorPool();
    void setupLayoutsAndDescriptors();

    void preparePipelines();


private:
    void prepareComputeBuffers();
    void prepareCommonComputeKernal();

private:

    //std::shared_ptr<VGContainer> _vgContainer;
    std::vector<glm::ivec4> outputIndex;

    struct {
        vk::TexelBuffer outputIndexBuffer;

        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
        VkPipelineLayout pipelineLayout;

        struct {
            VkPipeline scanline;
        }pipelines;
        
    } graphics;

    struct {
        PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR;

        VkVGInputCurveData curve_input;
        VkVGInputPathData path_input;

        VkQueue queue;
        VkCommandPool cmd_pool;

        struct {
            // transfromed
            VULKAN_BUFFER_PTR(vec2) transformed_pos;
            VULKAN_BUFFER_PTR(int32_t) path_visible;

            // monotonize
            VULKAN_BUFFER_PTR(int32_t) curve_pixel_count;
            VULKAN_BUFFER_PTR(float) monotonic_cutpoint_cache;
            VULKAN_BUFFER_PTR(uint32_t) monotonic_n_cuts_cache;
            VULKAN_BUFFER_PTR(float) intersection;


        } storage_buffers;


        TransPosIn trans_pos_in;
        MakeInteIn make_inte_in;

        struct {
            VULKAN_BUFFER_PTR(TransPosIn) k_trans_pos_ubo;
            VULKAN_BUFFER_PTR(MakeInteIn) k_make_inte_ubo;
        } uniform_buffers;

        // CPU-GPU synchronization
        VkFence fence;
    } _compute;

    struct {
        // common
        std::shared_ptr<ComputeKernal> scan;

        std::shared_ptr<ComputeKernal> transform_pos;
        std::shared_ptr<ComputeKernal> make_intersection_0;
        std::shared_ptr<ComputeKernal> make_intersection_1;
    } _kernal;


    VkPhysicalDevicePushDescriptorPropertiesKHR pushDescriptorProps{};

private:

    // const
    const std::string COMPUTE_SPV_DIR = "shaders/compute/spv/";
    const std::string COMMON_COMPUTE_SPV_DIR = "shaders/common/spv/";
    const int BLOCK_SIZE = 256;

};

}

#endif