#version 330 core

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4((inColor.rgb * 1.5), inColor.a);
}