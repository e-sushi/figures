#version 330 core

layout(std140) uniform UniformBufferObject{
	mat4  view;
	mat4  proj;
	vec4  lights[10];
	vec4  viewPos;
	vec2  screen;
	vec2  mousepos;
	vec3  mouseWorld;
	float time;
	mat4  depthMVP;
	int   enablePCF;
} ubo;

uniform PushConsts{
	mat4 model;
} primitive;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outNormal;

void main() {
    gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);
    outColor    = inColor;
	outUV       = inUV;
	outNormal   = mat3(primitive.model) * inNormal;
}