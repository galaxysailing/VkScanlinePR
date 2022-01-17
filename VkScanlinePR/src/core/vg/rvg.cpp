#include "rvg.h"
#include <fstream>
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

	std::cout << "parse success\n";

	return;
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

	// process each path loop
	while (fin >> tstr) {

		if (tstr[0] == '/' && tstr[1] == '/') {
			// ignore annotation
			getline(fin, tstr);
			continue;
		}

		assert(tstr == "1");
		fin >> tstr;
		assert(tstr == "element");
		
		// fill rule
		fin >> tstr;
		if (tstr == "ofill") {
			// TODO
		}
		else if (tstr == "nzfill") {

		}
		else {
			break;
		}

		fin >> tstr;
		assert(tstr == "dyn_concrete");

		read_point();
		read_point();

	}
}

};