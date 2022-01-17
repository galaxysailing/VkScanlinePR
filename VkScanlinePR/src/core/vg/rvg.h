#pragma once
#ifndef GALAXYSAILING_RVG_H_
#define GALAXYSAILING_RVG_H_

#include <string>
#include <memory>

#include "vg_container.h"

namespace Galaxysailing {

class RVG {
public:
	void load(const std::string& filename);

private:
	std::shared_ptr<VGContainer> _vgContainer;

	void parse_header(std::ifstream& fin);

	void parse_paths(std::ifstream& fin);
};

}

#endif