#version 330 core

uniform sampler2D uGrassTexture;

in vec2 fTexCoord;

out vec4 fColor;

void main(void)
{
    fColor = texture(uGrassTexture, fTexCoord);
}
