#version 450

layout (set = 1, binding = 0) uniform samplerCube uSamplerCubeMap;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = texture(uSamplerCubeMap, inUVW) - vec4(0.7, 0.7, 0.7, 0.0);
}