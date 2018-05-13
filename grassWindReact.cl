

// Adjusts grass blades in response to wind.
// The first two buffers will be both read and written.
__kernel void reactToWind(__global float *grassWindPositions,
                          __global float *grassWindVelocities,
                          __global float *grassWindStrength,
                          const unsigned int numBlades,
                          const float dt)
{
    int bladeIdx = get_global_id(0);


    if (bladeIdx < numBlades)
    {
        float windStrength = grassWindStrength[bladeIdx];

        // For now, the neutral position will be equal to the wind strength.
        float neutralPosition = windStrength;

        float position = grassWindPositions[bladeIdx];
        float velocity = grassWindVelocities[bladeIdx];

        const float damping = 0.999;
        const float dampingSquared = damping * damping;

        float sn, cs;
        sn = sincos(damping * dt, &cs);

        float hc = dampingSquared * neutralPosition;
        float x_minus_hc = position - hc;

        float newPosition = velocity * sn + x_minus_hc * cs + hc;
        float newVelocity = damping * (velocity * cs - x_minus_hc * sn);

        grassWindPositions[bladeIdx] = newPosition;
        grassWindVelocities[bladeIdx] = newVelocity;
    }
}
