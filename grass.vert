#version 330 core

uniform mat4 uMVP;

in vec3 vPosition;

void main(void)
{
    gl_Position = uMVP * vec4(vPosition, 1);
}
