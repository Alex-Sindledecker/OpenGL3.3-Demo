#version 330 core

in vec3 fragPos;
in vec2 uv;
in vec3 fnormal;
in vec4 fragPosLightSpace;

uniform sampler2D tex;
uniform sampler2D depthMap;
uniform vec3 lightDirection;

out vec4 color;

float getShadow()
{
	vec3 lightSpaceFrag = fragPosLightSpace.xyz / fragPosLightSpace.w;
	lightSpaceFrag = lightSpaceFrag * 0.5 + 0.5;

    vec2 texelSize = 1.f / textureSize(depthMap, 0);
    float shadow = 0.f;
    float bias = max(0.005 * (1.f - dot(normalize(fnormal), -normalize(vec3(-11, -5, -11)))), 0.005f);

    for (int x = -1; x < 1; x++)
    {
        for (int y = -1; y < 1; y++)
        {
            float pcfDepth = texture(depthMap, lightSpaceFrag.xy + vec2(x, y) * texelSize).r;
            shadow += lightSpaceFrag.z - bias > pcfDepth ? 0.f : 1.f;
        }
    }
    return shadow / 4.f;
}

void main()
{
	float ambientStrength = 0.35;
	vec3 ambient = vec3(ambientStrength);

	vec3 normal = normalize(fnormal);
	vec3 lightDir = -normalize(vec3(-11, -5, -11));
	float diff = max(dot(normal, lightDir), 0.f);
	vec3 diffuse = vec3(diff);

	color = vec4(ambient + diffuse * getShadow(), 1.f) * texture(tex, vec2(uv.x, 1 - uv.y));
    color = vec4(pow(color.rgb, vec3(1 / 2.2f)), 1.f);
}