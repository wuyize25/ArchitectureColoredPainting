#version 330 core

layout (location = 0) out vec4 gBaseColor;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gPosition;
layout (location = 3) out vec2 gMetallicRoughness;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

void main()
{      
    gBaseColor = vec4(TexCoords,1,1);
    gPosition = WorldPos;
    gNormal = Normal;
    gMetallicRoughness = vec2(0,0);
}