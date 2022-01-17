#pragma once
#ifndef GALAXYSAILING_VG_CONTAINER_H_
#define GALAXYSAILING_VG_CONTAINER_H_
#include <glm/glm.hpp>

#include <vector>
namespace Galaxysailing {

enum class CurveType {
	NONE = 0,
	LINE = 1,
	QUADRIC = 3,
	CUBIC = 4,
	ARC = 7
};
enum class FillRule {
	NONE = 0,
	NON_ZERO = 1,
	EVEN_ODD = 2
};

struct VGContainer {
public:
	glm::vec4 vp;
	glm::vec4 win;

	struct PointData {
		std::vector<glm::vec2> pos;
	};

	struct CurveData {
		std::vector<uint32_t> posIndices;
		std::vector<CurveType> curveType;
		int curveIndex = -1;
	};
	
	struct PathData {
		std::vector<uint32_t> curveIndices;

		std::vector<FillRule> fillRule;
		std::vector<glm::vec4> fillColor;
		std::vector<float> fillOpacity;
		int pathIndex = -1;
	};

	PathData pathData;
	CurveData curveData;
	PointData pointData;
public:
	void newPath() {
		++pathData.pathIndex;
		pathData.curveIndices.push_back(curveData.curveIndex + 1);
		pathData.fillRule.push_back(FillRule::NONE);
		pathData.fillColor.push_back(glm::vec4(0, 0, 0, 1));
		pathData.fillOpacity.push_back(0.0f);

	}

	void newCurve() {
		++curveData.curveIndex;
		curveData.posIndices.push_back(pointData.pos.size());
		curveData.curveType.push_back(CurveType::NONE);
	}

public:

	void addCurve(CurveType ct, glm::vec2 *p) {
		auto& curInd = curveData.curveIndex;
		curveData.curveType[curInd] = ct;
		switch (ct) {
		case CurveType::LINE:
			pointData.pos.push_back(p[0]);
			pointData.pos.push_back(p[1]);
			break;
		case CurveType::CUBIC:
			pointData.pos.push_back(p[0]);
			pointData.pos.push_back(p[1]);
			pointData.pos.push_back(p[2]);
			pointData.pos.push_back(p[3]);
			break;
		default: break;
		}
	}
};

}
#endif