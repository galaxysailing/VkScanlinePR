#pragma once
#ifndef GALAXYSAILING_VG_CONTAINER_H_
#define GALAXYSAILING_VG_CONTAINER_H_
#include <glm/glm.hpp>

#include <vector>
namespace Galaxysailing {
enum CurveType {
	LINE = 1,
	QUADRIC = 3,
	CUBIC = 4,
	ARC = 7
};
enum FillRule {
	NON_ZERO = 0,
	EVEN_ODD = 1
};
struct VGContainer {
	glm::vec4 vp;
	glm::vec4 win;

	struct VertexData {
		std::vector<glm::vec2> pos;
	};

	struct CurveData {
		std::vector<uint32_t> posIndices;
		std::vector<CurveType> curveType;
	};
	
	struct PathData {
		std::vector<uint32_t> curveIndices;
		std::vector<FillRule> fillRule;
		std::vector<glm::vec4> color;
		std::vector<float> opacity;
	};
};

}
#endif