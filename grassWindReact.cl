

// Adjusts grass blades in response to wind.
// The first two buffers will be both read and written.
__kernel void reactToWind(__global float2 *grassWindPositions,
                          __global float2 *grassWindVelocities,
                          __global float2 *grassWindStrength,
                          const unsigned int numBlades,
                          const float dt)
{
    int bladeIdx = get_global_id(0);


    if (bladeIdx < numBlades)
    {
        float2 windStrength = grassWindStrength[bladeIdx];

        // For now, the neutral position will be equal to the wind strength.
        float2 neutralPosition = windStrength;

        float2 position = grassWindPositions[bladeIdx];
        float2 velocity = grassWindVelocities[bladeIdx];

        const float damping = 0.999;
        const float dampingSquared = damping * damping;

        float sn, cs;
        sn = sincos(damping * dt, &cs);

        float2 hc = dampingSquared * neutralPosition;
        float2 x_minus_hc = position - hc;

        float2 newPosition = velocity * sn + x_minus_hc * cs + hc;
        float2 newVelocity = damping * (velocity * cs - x_minus_hc * sn);

        grassWindPositions[bladeIdx] = newPosition;
        grassWindVelocities[bladeIdx] = newVelocity;
    }
}
