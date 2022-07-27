#version 450 core

 
uniform sampler2D texture_basecolor;
uniform sampler2D texture_metallic_roughness;
uniform sampler2D texture_normal;

layout (location = 0) out vec4 gBaseColor;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gPosition;
layout (location = 3) out vec2 gMetallicRoughness;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;



vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(texture_normal, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T   = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B   = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main()
{      
    //gBaseColor = vec4(1,0,0,1);
    gBaseColor = texture(texture_basecolor, TexCoords);
    if(gBaseColor.a<0.4)
        discard;
    gPosition = WorldPos;
    gNormal = getNormalFromMap();
    gMetallicRoughness = texture(texture_metallic_roughness, TexCoords).bg;


}