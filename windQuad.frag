#version 330 core

uniform sampler2D uWindSpeed;

in vec2 fTexCoord;

out vec4 fragColor;

void main()
{
    vec2 windSpeed = texture(uWindSpeed, fTexCoord).xy;

    const vec2 mean = vec2(0, 0);
    const float std = 0.5;

    vec2 visual = (windSpeed - mean) / (2 * std) + vec2(0.5, 0.5);

    fragColor = vec4(visual, 0, 1);
}
