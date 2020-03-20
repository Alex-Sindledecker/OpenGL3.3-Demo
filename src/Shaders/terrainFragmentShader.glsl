#version 330 core

in vec3 fragPos;
in vec2 uv;
in vec3 fnormal;
in vec4 fragPosLightSpace;

out vec4 color;

uniform sampler2D textureMap;
uniform sampler2D texture_r;
uniform sampler2D texture_g;
uniform sampler2D texture_b;
uniform sampler2D depthMap;
uniform int terrainSize;

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
    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * vec3(1);

    vec3 normal = normalize(fnormal);
    vec3 lightDir = -normalize(vec3(-11, -5, -11));
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1);

    vec3 mapRGB = texture(textureMap, uv).rgb;
    vec3 rTex = texture(texture_r, uv * terrainSize).rgb;
    vec3 gTex = texture(texture_g, uv * terrainSize).rgb;
    vec3 bTex = texture(texture_b, uv * terrainSize).rgb;
    float rPct = mapRGB.r;
    float gPct = mapRGB.g;
    float bPct = mapRGB.b;
    vec3 fragColor = rTex * rPct + gTex * gPct + bTex * bPct;

    vec4 result = vec4(ambient + diffuse * getShadow(), 1.f) * vec4(fragColor, 1.f);
	color = result;
    color = vec4(pow(color.rgb, vec3(1 / 2.2f)), 1.f);
}