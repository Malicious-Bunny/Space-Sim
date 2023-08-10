#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

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
	outUVW = inPosition;

	vec4 positionWorld = push.modelMatrix * vec4(inPosition, 1.0);
    gl_Position = ubo.projectionView * positionWorld;
}
