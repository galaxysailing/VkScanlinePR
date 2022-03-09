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

	void init(const glm::vec3& pos, bool flip) {
		_position = pos;
		_up = glm::vec3(
			0.0f
			, flip ? -1.0f : 1.0f
			, 0.0f);

		_mv = glm::mat4(1.0f);
	}

	// controller interface

	void rightBtnDown() {
		keyDown();
		_right_btn_down = true;
	}
	void rightBtnUp() {
		keyUp();
		_right_btn_down = false;
	}

	void move(float x, float y) {
		if (_right_btn_down) {
			float dx = x - _last_pos.x, dy = _last_pos.y - y;
			translate(dx, dy);
		}
		_last_pos.x = x;
		_last_pos.y = y;
	}

	void wheel(float dy) {
		if (_key_down) {
			return;
		}
		translate(-_last_pos.x, -_last_pos.y);
		scale(dy > 0 ? 1.04f : 0.96f);
		translate(_last_pos.x, _last_pos.y);
	}

public:
	glm::mat4 mv() {
		return _mv;
	}

private:
	glm::vec3 _position;
	glm::vec3 _up;

private:
	bool _right_btn_down = false;
	bool _key_down = false;
	glm::vec2 _last_pos = glm::vec2(0.0f);

private:
	void keyDown() {
		_key_down = true;
	}

	void keyUp() {
		_key_down = false;
	}

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

	void scale(float s) {
		//_mv = glm::scale(_mv, glm::vec2(x, y));
		_mv = glm::mat4(
			s, 0, 0, 0,
			0, s, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1) * _mv;
	}

private:
	glm::mat4 _mv;
};

};
#endif