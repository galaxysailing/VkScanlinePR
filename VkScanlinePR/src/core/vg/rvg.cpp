#include "rvg.h"
#include <fstream>
#include <sstream>
#include <iostream>

#include <glm/glm.hpp>
namespace Galaxysailing {

void RVG::load(const std::string& filename)
{
	std::ifstream fin;
	fin.open(filename);
	if (!fin.is_open()) {
		throw std::runtime_error("RVG::load can't open file \"" + filename + "\"");
	}
	
	_vgContainer = std::make_shared<VGContainer>();

	parse_header(fin);

	parse_paths(fin);

	std::cout << "---------- vg load success ---------\n";

	return;
}

std::shared_ptr<VGContainer> RVG::getVGContainer()
{
	return _vgContainer;
}

void RVG::parse_header(std::ifstream &fin)
{
	auto& vg = *_vgContainer;

	std::string tstr;
	auto replace_comma = [](std::string& str) {
		for (auto& c : str) { if (c == ',') { c = ' '; } }
	};

	auto replace_colon = [](std::string& str) {
		for (auto& c : str) { if (c == ':') { c = ' '; } }
	};

	// [&]: use external value by 'reference'
	auto read_point = [&]() -> glm::vec2 {
		std::string istr;
		fin >> istr;
		replace_comma(istr);
		replace_colon(istr);

		glm::vec2 v;
		sscanf_s(istr.c_str(), "%f %f", &v.x, &v.y);

		return v;
	};
	glm::vec2 xy, zw;

	// viewport
	fin >> tstr;
	xy = read_point();
	zw = read_point();
	vg.vp[0] = xy.x;
	vg.vp[1] = xy.y;
	vg.vp[2] = zw.x;
	vg.vp[3] = zw.y;

	// window
	fin >> tstr;
	xy = read_point();
	zw = read_point();
	vg.win[0] = xy.x;
	vg.win[1] = xy.y;
	vg.win[2] = zw.x;
	vg.win[3] = zw.y;

	fin >> tstr;
	assert(tstr == "scene");

	fin >> tstr;
	assert(tstr == "dyn_identity");
}

void RVG::parse_paths(std::ifstream& fin)
{
	auto& vg = *_vgContainer;

	std::string tstr;
	auto replace_comma = [](std::string& str) {
		for (auto& c : str) { if (c == ',') { c = ' '; } }
	};

	auto replace_colon = [](std::string& str) {
		for (auto& c : str) { if (c == ':') { c = ' '; } }
	};

	// [&]: use external value by 'reference'
	auto read_point = [&]() -> glm::vec2 {
		std::string istr;
		fin >> istr;
		replace_comma(istr);
		replace_colon(istr);

		glm::vec2 v;
		sscanf_s(istr.c_str(), "%f %f", &v.x, &v.y);

		return v;
	};
	int last_path_vertex_number = -1;

	// process each path loop
	while (fin >> tstr) {

		// process header
		if (tstr[0] == '/' && tstr[1] == '/') {
			// ignore annotation
			getline(fin, tstr);
			continue;
		}
		if (last_path_vertex_number != vg.pointData.pos.size()) {
			last_path_vertex_number = vg.pointData.pos.size();
			vg.newPath();
		}
		//printf("path idx %d begin\n", vg.pathData.pathIndex);

		assert(tstr == "1");
		fin >> tstr;
		assert(tstr == "element");
		
		auto& pathInd = vg.pathData.pathIndex;
		// fill rule
		fin >> tstr;
		if (tstr == "ofill") {
			vg.pathData.fillRule[pathInd] = FillRule::EVEN_ODD;
		}
		else if (tstr == "nzfill") {
			vg.pathData.fillRule[pathInd] = FillRule::NON_ZERO;
		}
		else {
			fin >> tstr;
			continue;
		}

		fin >> tstr;
		assert(tstr == "dyn_concrete");

		read_point();
		read_point();
		read_point();

		glm::vec2 p[4];

		int path_vertex_begin = vg.pointData.pos.size();
		int contour_vertex_begin = path_vertex_begin;
		bool closed = false;

		// process curve
		while (fin >> tstr) {
			if (tstr != "fL" && tstr.length() != 1) {
				break;
			}
			
			char cmd = tstr[0];
			if (std::string("MmZzLlCcAa").find(cmd) == std::string::npos) {
				if (tstr != "fL") {
					break;
				}
			}

			//auto& curveInd = vg.curveData.curveIndex;

			switch (cmd) {
			case 'M':
				p[0] = read_point();
				contour_vertex_begin = vg.pointData.pos.size();
				closed = false;
				break;
			case 'L':
				p[1] = read_point();
				vg.newCurve();
				vg.addCurve(CurveType::LINE, p);
				p[0] = p[1];
				break;
			case 'C':
				p[1] = read_point();
				p[2] = read_point();
				p[3] = read_point();
				vg.newCurve();
				vg.addCurve(CurveType::CUBIC, p);
				p[0] = p[3];
				break;
			case 'Z':
				closed = true;
				break;
			default: 
				if (tstr == "fL") {
					p[0] = read_point();
				}
				break;

			}
		}

		//if (vg.pointData.pos.size() == path_vertex_begin) {
		//	getline(fin, tstr);
		//	continue;
		//}

		// close path
		if (!closed && vg.pointData.pos.size() > contour_vertex_begin) {
			// add a straight line.
			glm::vec2 p_first = vg.pointData.pos[contour_vertex_begin];
			glm::vec2 p_last = vg.pointData.pos.back();

			if (p_first != p_last) {
				vg.newCurve();
				p[0] = p_last;
				p[1] = p_first;
				vg.addCurve(CurveType::LINE, p);
			}

		}

		assert(tstr == "dyn_identity");
		fin >> tstr;
		assert(tstr == "dyn_paint");
		
		float opacity;
		fin >> opacity;
		vg.pathData.fillOpacity[pathInd] = opacity;

		fin >> tstr;
		if (tstr == "solid") {

			fin >> tstr;
			if (tstr.substr(0, 4) == "rgba") {
				tstr = tstr.substr(5, tstr.length() - 6);
				replace_comma(tstr);
				std::istringstream iss(tstr);
				glm::vec4 &color = vg.pathData.fillColor[pathInd];
				iss >> color.r >> color.g >> color.b >> color.a;
			}
			else if (tstr.substr(0, 3) == "rgb") {
				tstr = tstr.substr(4, tstr.length() - 6);
				replace_comma(tstr);
				std::istringstream iss(tstr);
				glm::vec4& color = vg.pathData.fillColor[pathInd];
				iss >> color.r >> color.g >> color.b;
			}
		}
		//printf("path idx %d end\n", vg.pathData.pathIndex);

	}
}

};