#version 450
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec2 outTexCoords;

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
    outTexCoords = inTexCoords;
    vec3 posWorld = vec3(push.modelMatrix * vec4(inPos, 1.0));
	gl_Position = ubo.projectionView * vec4(posWorld, 1.0);
}