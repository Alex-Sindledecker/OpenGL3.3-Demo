#version 330 core

layout (location = 0) in vec3 pos;

uniform mat4 lightSpaceTransform;
uniform mat4 models[50];

void main()
{
	gl_Position = lightSpaceTransform * models[gl_InstanceID] * vec4(pos, 1.f);
}