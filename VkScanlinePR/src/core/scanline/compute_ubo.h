#pragma once
#ifndef GALAXYSAILING_COMPUTE_UBO_H_
#define GALAXYSAILING_COMPUTE_UBO_H_

#include <cstdint>
#include <glm/glm.hpp>

namespace Galaxysailing {
struct TransPosIn {
    uint32_t n_points;
    float w, h;
    alignas(16) glm::vec4 m0, m1, m2, m3;
};

struct MakeInteIn {
    uint32_t n_curves;
    uint32_t n_fragments;
    int w, h;
};
};

#endif
