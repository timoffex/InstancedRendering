#version 330 core

uniform mat4 uMVP;

in vec3 vPosition;
in vec3 vOffset;

void main(void)
{
    gl_Position = uMVP * vec4(vPosition + vOffset, 1);
}
