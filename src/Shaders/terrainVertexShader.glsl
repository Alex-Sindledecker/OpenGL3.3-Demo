#version 330 core

layout (location = 0) in vec3 vpos;
layout (location = 1) in vec3 vnormal;

out vec3 fnormal;
out vec3 fragPos;
out vec2 uv;
out vec4 fragPosLightSpace;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 lightSpaceTransform;

void main()
{
	gl_Position = projection * view * model * vec4(vpos, 1.f);
	fnormal = mat3(transpose(inverse(model))) * vnormal;
	fragPos = vec3(model * vec4(vpos, 1.f));
	uv = vpos.xz;
	fragPosLightSpace = lightSpaceTransform * vec4(fragPos, 1.f);
}