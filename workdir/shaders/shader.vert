#version 450
#extension GL_KHR_vulkan_glsl: enable

layout(binding = 0) uniform UniformBufferObject {
    vec2 foo;
    mat4 model;
    mat4 view;
    mat4 proj;

} ubo;

layout(binding = 2) uniform samplerBuffer outputIndex;
//layout(location = 0) in vec3 inPosition;
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
//vec2 positions[3] = vec2[](
//    vec2(0.0, -0.5),
//    vec2(0.5, 0.5),
//    vec2(-0.5, 0.5)
//);
//
//vec3 colors[3] = vec3[](
//    vec3(1.0, 0.0, 0.0),
//    vec3(0.0, 1.0, 0.0),
//    vec3(0.0, 0.0, 1.0)
//);

void main() {
//    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
//    fragColor = colors[gl_VertexIndex];

//    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
    // gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    //  gl_Position = vec4(vertex.xy,0.0,1.0);

    vec4 vertex = texelFetch(outputIndex, gl_VertexIndex);
   gl_Position = ubo.proj * ubo.view * ubo.model * vec4(vertex.xy,0.0,1.0);

//    fragColor = vec3(texelFetch(outputIndex, gl_VertexIndex));
    fragTexCoord = vertex.zw;
}