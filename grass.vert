#version 330 core

uniform mat4 uMVP;
uniform float uWindStrength;

in vec3 vPosition;
in vec2 vTexCoord;

in vec3 vOffset;
in mat3 vRotation;

out vec2 fTexCoord;

void main(void)
{
    vec3 rotated = vRotation * vPosition;


    // Offset higher vertices in grass blade model to create
    // a "tilting in the wind" effect. Do this after rotating
    // the blade so that all blades tilt in the same direction.
    vec3 tilted = rotated;
    tilted.x += vPosition.y * uWindStrength;

    gl_Position = uMVP * vec4(tilted + vOffset, 1);

    fTexCoord = vTexCoord;
}
