

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

        float acceleration = neutralPosition - position;

        // second order approximation
        float newPosition = position + dt * velocity + 0.5 * dt * dt * acceleration;
        float newVelocity = velocity + dt * acceleration;

        grassWindPositions[bladeIdx] = newPosition;
        grassWindVelocities[bladeIdx] = newVelocity;
    }
}
