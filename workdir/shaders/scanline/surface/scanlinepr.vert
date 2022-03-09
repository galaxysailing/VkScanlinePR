#version 450
layout(binding = 0) uniform isamplerBuffer tb_index;

// flat: no interpolation is done
layout(location = 0)flat out vec4 fragment_color;
layout(location = 1)flat out ivec2 path_frag_pos;

vec4 u8rgba2frgba(int c) {
	return vec4(c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF, (c >> 24) & 0xFF) / 255.0;
}

void calc_color(int colori) {
	vec4 color = u8rgba2frgba(colori);
	fragment_color = color;
}

// uniform vec2 vp_size;
void main() {
	int index = gl_VertexIndex >> 1;
	int line_vi = gl_VertexIndex & 1;

	// x: yx
	// y: width
	// z: fillinfo
	// w: frag_index / 0
	ivec4 draw = texelFetch(tb_index, index);

	path_frag_pos = ivec2(draw.x & 0xFFFF, (draw.x >> 16) & 0xFFFF);

	vec2 pos = vec2(
		path_frag_pos.x + line_vi * draw.y,
		path_frag_pos.y
	);

	calc_color(draw.z);

	pos.y += 1;

    ivec2 vp_size = ivec2(1200, 1024);

	pos.x = pos.x / float(vp_size.x) * 2 - 1.0;
	pos.y = pos.y / float(vp_size.y) * 2 - 1.0;

	pos.y = -pos.y;

	gl_Position = vec4(pos, 0, 1);

	//pixel_mask = (draw.w == 0) ?
	//	ivec4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF)
	//	: texelFetch(tb_stencil_mask, draw.w - 1);
}