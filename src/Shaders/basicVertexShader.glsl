#version 330 core

layout (location = 0) in vec3 pos;

uniform mat4 proj = mat4(1.f);

out vec2 uv;

void main()
{
	gl_Position = proj * vec4(pos, 1.f).xyzw;
	uv = vec2(pos.x, pos.y);
}