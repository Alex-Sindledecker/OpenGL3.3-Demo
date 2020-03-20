#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec2 v_uv;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 lightSpaceTransform;
uniform mat4 models[50];

out vec3 fragPos;
out vec2 uv;
out vec3 fnormal;
out vec4 fragPosLightSpace;

void main()
{
	gl_Position = projection * view * models[gl_InstanceID] * vec4(pos, 1.f);
	uv = v_uv;
	fnormal = mat3(transpose(inverse(models[gl_InstanceID]))) * v_normal;
	fragPos = vec3(models[gl_InstanceID] * vec4(pos, 1.f));
	fragPosLightSpace = lightSpaceTransform * vec4(fragPos, 1.f);
}