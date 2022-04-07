#include "vg_app.h"
#include "../core/rasterizer.h"
#include "../core/scanline/scanline_rasterizer.h"
#include "../core/vg/rvg.h"
#include "../core/common/camera.hpp"
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
	bool _init;

	// memory management by GFLW
	GLFWwindow *_window;

	std::shared_ptr<VGContainer> _vgContainer;

	std::shared_ptr<VGRasterizer> _vgRasterizer;

	Camera _camera;

// for user operation
private:
	struct {
		bool right_down = false;
		bool left_down = false;
		glm::ivec2 pos;
	} _mouse_state;

	static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);	
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
};


// ------------------------------ implement of inner function ----------------------------
void ScanlineVGApplication::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	_window = glfwCreateWindow(_width, _height, _appName.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(_window, this);

	//mouse
	glfwSetCursorPosCallback(_window, ScanlineVGApplication::cursorPosCallback);
	glfwSetScrollCallback(_window, ScanlineVGApplication::scrollCallback);
	glfwSetMouseButtonCallback(_window, ScanlineVGApplication::mouseButtonCallback);

	//glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
}
void ScanlineVGApplication::cleanupWindow()
{
	glfwDestroyWindow(_window);

	glfwTerminate();
}

// ------------------------------ user operation ---------------------------------------
void ScanlineVGApplication::cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	auto& app = *((ScanlineVGApplication*)glfwGetWindowUserPointer(window));
	auto& camera = app._camera;
	auto& mouse_state = app._mouse_state;
	//printf("window size: %d, %d\n", that->_width, that->_height);	

	if (app._mouse_state.right_down) {
		float dx = xpos - mouse_state.pos.x, dy = mouse_state.pos.y - ypos;
		camera.translate(dx, dy);
	}
	app._mouse_state.pos = glm::ivec2(xpos, ypos);

	printf("mouse pos: %f, %f\n", xpos, ypos);
	//app._camera.move(xpos, ypos);

	//glm::mat4 m = glm::transpose(app._camera.mv());
	//printf("%f %f %f %f\n", m[0][0], m[0][1], m[0][2], m[0][3]);
	//printf("%f %f %f %f\n", m[1][0], m[1][1], m[1][2], m[1][3]);
	//printf("%f %f %f %f\n", m[2][0], m[2][1], m[2][2], m[2][3]);
	//printf("%f %f %f %f\n", m[3][0], m[3][1], m[3][2], m[3][3]);

}

void ScanlineVGApplication::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	auto& app = *((ScanlineVGApplication*)glfwGetWindowUserPointer(window));
	auto& camera = app._camera;
	auto& mouse_state = app._mouse_state;

	auto cp = mouse_state.pos;
	cp.y = app._height - cp.y;

	float s = yoffset > 0 ? 1.1f : 0.9f;
	camera.translate(-cp.x, -cp.y);
	camera.scale(s);
	camera.translate(cp.x, cp.y);
	//app._camera.wheel(yoffset);
	
	printf("mouse scroll: %f, %f\n", xoffset, yoffset);
}

void ScanlineVGApplication::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	auto& app = *((ScanlineVGApplication*)glfwGetWindowUserPointer(window));
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		switch (action) {
			//case GLFW_PRESS:app._camera.rightBtnDown(); break;
			//case GLFW_RELEASE:app._camera.rightBtnUp(); break;
			case GLFW_PRESS: app._mouse_state.right_down = true; break;
			case GLFW_RELEASE: app._mouse_state.right_down = false; break;
		}
	}
}

// ------------------------------ implement of public function ---------------------------
void ScanlineVGApplication::run() {
	if (!_init) {
		initWindow();
		_camera.init(_width, _height);
		_vgRasterizer = std::make_shared<ScanlineVGRasterizer>();
		_vgRasterizer->initialize(_window, _width, _height);
		_vgRasterizer->loadVG(_vgContainer);
		_init = true;
	}

	while (!glfwWindowShouldClose(_window)) {
		glfwPollEvents();

		glm::mat4 m = _camera.mv();
		_vgRasterizer->setMVP(glm::transpose(m));

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