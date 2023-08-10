#version 450

layout(set = 1, binding = 0) uniform sampler2D uAlbedoMap;
layout(set = 1, binding = 1) uniform sampler2D uNormalMap;
layout(set = 1, binding = 2) uniform sampler2D uMetallicMap;
layout(set = 1, binding = 3) uniform sampler2D uRoughnessMap;

layout(location = 0) in vec2 inTexCoords;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = texture(uAlbedoMap, inTexCoords);
}