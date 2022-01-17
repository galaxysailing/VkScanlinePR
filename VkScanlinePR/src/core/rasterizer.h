#pragma once
#ifndef GALAXYSAILING_RASTERIZER_H_
#define GALAXYSAILING_RASTERIZER_H_

#include "vg/vg_container.h"

namespace Galaxysailing {

class VGRasterizer {
public:
    virtual void initialize(void* window, uint32_t w, uint32_t h) = 0;

    virtual void render() = 0;

    virtual void loadVG(std::shared_ptr<VGContainer> vg) = 0;

	//virtual void viewport(int x, int y, int w, int h) = 0;
};


}

#endif