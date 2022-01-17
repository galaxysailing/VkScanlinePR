#include "rasterizer.h"

#include <stdexcept>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <array>
#include <iostream>
#include <assert.h>

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

    _prepared = true;
}

// ------------------------------ ---------------------
void ScanlineVGRasterizer::drawFrame()
{
}

}