#pragma once
#ifndef GALAXYSAILING_RASTERIZER_H_
#define GALAXYSAILING_RASTERIZER_H_

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <string>

#include "vk/vk_device.h"
#include "vk/vk_swapchain.h"
#include "vk/vk_util.h"
#include "vk/vk_debug.h"
#include "vk/vk_vg_rasterizer.h"
namespace Galaxysailing {

class VGRasterizer {
public:
    virtual void initialize(void* window, uint32_t w, uint32_t h) = 0;

    virtual void render() = 0;

	//virtual void viewport(int x, int y, int w, int h) = 0;
};

#include <GLFW/glfw3.h>

class ScanlineVGRasterizer : public VulkanVGRasterizerBase, public VGRasterizer {
// ----------------------------- rasterizer interface ------------------------
public:

    void initialize(void* window, uint32_t w, uint32_t h) override;

	void render() override;

    //void viewport(int x, int y, int w, int h) override;

    ~ScanlineVGRasterizer() {
        VK_CHECK_RESULT(vkDeviceWaitIdle(_device));
    }

// ------------------------------ Vulkan Base override ------------------------
private:
    
    void prepare() override;

private:
    
    void drawFrame();


};


}

#endif