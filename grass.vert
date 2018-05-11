#version 330 core

uniform mat4 uMVP;

in vec3 vPosition;
in vec2 vTexCoord;

in vec3 vOffset;
in mat3 vRotation;

out vec2 fTexCoord;

void main(void)
{
    gl_Position = uMVP * vec4(vRotation * vPosition + vOffset, 1);

    fTexCoord = vTexCoord;
}
