#pragma once
#ifndef GALAXYSAILING_CAMERA_H_
#define GALAXYSAILING_CAMERA_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Galaxysailing {

class Camera {
public:
	Camera() {}

	void init(glm::vec3 pos, bool flip) {
		_position = pos;
		_up = glm::vec3(
			0.0f
			, flip ? -1.0f : 1.0f
			, 0.0f);
	}

	// controller interface
	void rightBtnDown() {
		_right_btn_down = true;
	}
	void rightBtnUp() {
		_right_btn_down = false;
	}

	void move(float x, float y) {
		if (_right_btn_down) {
			_position += glm::vec3(x, y, 0);
			translate(_position.x, _position.y);
		}
	}

	void wheel() {

	}

private:
	glm::vec3 _position;
	glm::vec3 _up;
private:
	bool _right_btn_down;

private:

	void reset() {
		_mv = glm::mat4(1.0f);
	}

	glm::mat4 getModelView() {
		return _mv;
	}

	void translate(float x, float y) {
		_mv = glm::mat4(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			x, y, 0, 1) * _mv;
	}

	void scale(float x, float y) {
		//_mv = glm::scale(_mv, glm::vec2(x, y));
		_mv = glm::mat4(
			x, 0, 0, 0,
			0, y, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1) * _mv;
	}

private:
	glm::mat4 _mv;
};

};
#endif