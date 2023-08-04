#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout (location = 0) out vec3 outUVW;

layout(set = 0, binding = 0) uniform GlobalUbo
{
    mat4 projectionView;
} ubo;

layout(push_constant) uniform Push
{
    mat4 modelMatrix;
} push;

void main() 
{
	outUVW = position;

	vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
    gl_Position = ubo.projectionView * positionWorld;
}
