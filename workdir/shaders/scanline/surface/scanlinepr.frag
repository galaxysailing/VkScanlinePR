#version 450

layout(location = 0)flat in vec4 fragment_color;
layout(location = 1)flat in ivec2 path_frag_pos;

layout(location = 0) out vec4 out_color;
void main() {

	ivec2 in_frag_pos = ivec2(gl_FragCoord.xy) - path_frag_pos;

	out_color = fragment_color;

	// int mask_index = in_frag_pos.x * 2 + in_frag_pos.y;
	gl_SampleMask[0] = 0xFFFFFFFF;
} 