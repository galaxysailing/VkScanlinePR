#include "vg_app.h"
#include "../core/rasterizer.h"
#include "../core/scanline/scanline_rasterizer.h"
#include "../core/vg/rvg.h"
#include <string>
#include <memory>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

namespace Galaxysailing {

class ScanlineVGApplication;
std::shared_ptr<Galaxysailing::ScanlineVGApplication> app;

/*
* Scanline VG App
*/
class ScanlineVGApplication : public VGApplication {
public:
	ScanlineVGApplication() :_appName(""), _width(0), _height(0), _init(false) {}
	~ScanlineVGApplication() { cleanupWindow(); }
public:
	void run() override;
	VGApplication* viewport(int x, int y, int w, int h) override;
	VGApplication* appName(const char* name) override;

	VGApplication* loadPathFile(const char* filename) override;

// ------------------------------ inner function -----------------------------------------
private:
	void initWindow();
	void cleanupWindow();
private:
	std::string _appName;
	int _width, _height;

	// memory management by GFLW
	GLFWwindow *_window;

	std::shared_ptr<VGContainer> _vgContainer;

	std::shared_ptr<VGRasterizer> _vgRasterizer;

	bool _init;
};


// ------------------------------ implement of inner function ----------------------------
void ScanlineVGApplication::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	_window = glfwCreateWindow(_width, _height, _appName.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(_window, this);
	//glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
}
void ScanlineVGApplication::cleanupWindow()
{
	glfwDestroyWindow(_window);

	glfwTerminate();
}
// ------------------------------ implement of public function ---------------------------
void ScanlineVGApplication::run() {
	if (!_init) {
		initWindow();
		_vgRasterizer = std::make_shared<ScanlineVGRasterizer>();
		_vgRasterizer->initialize(_window, _width, _height);
		_vgRasterizer->loadVG(_vgContainer);
		_init = true;
	}

	while (!glfwWindowShouldClose(_window)) {
		glfwPollEvents();

		_vgRasterizer->render();
	}

}

VGApplication* ScanlineVGApplication::appName(const char* name) {
	_appName = std::string(name);
	return this;
}

VGApplication* ScanlineVGApplication::loadPathFile(const char* filename)
{
	// TODO load path file
	std::string fileStr(filename);
	int ind = fileStr.find_last_of('.');
	if (ind == std::string::npos 
		|| ind + 3 >= fileStr.size() 
		|| fileStr[ind + 2] != 'v'
		|| fileStr[ind + 3] != 'g') {
		throw std::runtime_error("input path file is illegal: " + fileStr);
	}

	++ind;
	if (fileStr[ind] == 'r') {
		RVG rvg;
		rvg.load(fileStr);
		_vgContainer = rvg.getVGContainer();
	}
	
	return this;
}

VGApplication* ScanlineVGApplication::viewport(int x, int y, int w, int h) {
	if (_vgRasterizer != nullptr) {
		//_vgRasterizer->viewport(x, y, w, h);
	}
	_width = w;
	_height = h;
	//printf("viewX: %d, viewY: %d, width: %d, height:%d\n", x, y, _width, _height);
	return this;
}

}// end of namespace Galaxysailing


std::shared_ptr<VGApplication> getAppInstance() {
	using Galaxysailing::app;
	if (app == nullptr) {
		app = std::make_shared<Galaxysailing::ScanlineVGApplication>();
	}
	return app;
}