
__kernel void reactToWind2(__global float2 *grassWindOffsets,           // The tilt for each grass blade is here.
                           __global float *grassPeriodOffsets,          // A time offset for each grass blade to make them less synchronized.
                           __global float2 *grassNormalizedPositions,   // For each grass blade, a corresponding position in the wind.
                           __read_only image2d_t windVelocityImg,       // The image containing wind velocities.
                           const unsigned int numBlades,                // Number of grass blades.
                           const float time)                            // The current time in seconds.
{
    unsigned int bladeIdx = get_global_id(0);

    const sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE  |
                              CLK_ADDRESS_CLAMP           |
                              CLK_FILTER_LINEAR;

    if (bladeIdx < numBlades)
    {
        const float maxOffset = 1;
        const float vibrationMagnitude = 0.3f;
        const float vibrationFrequency = 10; // angular frequency in 2*pi hertz

        float2 normalizedCoords = grassNormalizedPositions[bladeIdx];
        float2 windVelocity = read_imagef(windVelocityImg, sampler, normalizedCoords).xy;

        float windStrength = fast_length(windVelocity);
        float2 windDirection = (float2) (0, 0);
        if (windStrength > 0)
            windDirection = windVelocity / windStrength;

        float2 neutralOffset = windDirection * tanh(windStrength) * maxOffset;

        float timeOffset = grassPeriodOffsets[bladeIdx];
        float vibrationTime = vibrationFrequency * (time + timeOffset);
        float2 vibrationOffset = windDirection * sin(vibrationTime) * vibrationMagnitude * min(windStrength, 2.0f);

        grassWindOffsets[bladeIdx] = neutralOffset + vibrationOffset;
    }
}
