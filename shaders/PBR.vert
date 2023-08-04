#version 450
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec2 outTexCoords;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec4 outPosLightSpace;

layout(set = 0, binding = 0) uniform GlobalUbo
{
    mat4 projectionView;
	mat4 lightMatrix;
} ubo;

layout(push_constant) uniform Push
{
    mat4 modelMatrix;
	mat4 normalMatrix;
} push;

void main() {
	outTexCoords = inTexCoords;
	outWorldPos  = vec3(push.modelMatrix * vec4(inPos, 1.0));
	outNormal    = mat3(push.normalMatrix) * inNormal;
	outPosLightSpace = ubo.lightMatrix * vec4(outWorldPos, 1.0);

	gl_Position = ubo.projectionView * vec4(outWorldPos, 1.0);
}
