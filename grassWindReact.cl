
/* Performs a Jacobi iteration:
    output(i,j) = [input(i-1,j) + input(i+1,j) + input(i,j-1) + input(i,j+1) + alpha*b(i,j)] * betaInverse
*/


__kernel void jacobi(__read_only image2d_t input,
                     __read_only image2d_t b,
                     __write_only image2d_t output,
                     const float alpha,
                     const float betaInverse)
{
    const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                              CLK_ADDRESS_CLAMP           |
                              CLK_FILTER_NEAREST;

    int2 coords = (int2) (get_global_id(0), get_global_id(1));

    if (coords.x < get_image_width(output) && coords.y < get_image_height(output))
    {
        float4 outVal = (read_imagef(input, sampler, (int2)(coords.x-1, coords.y))
                        +read_imagef(input, sampler, (int2)(coords.x+1, coords.y))
                        +read_imagef(input, sampler, (int2)(coords.x, coords.y-1))
                        +read_imagef(input, sampler, (int2)(coords.x, coords.y+1))
                        +alpha * read_imagef(b, sampler, coords)) * betaInverse;
        write_imagef(output, coords, outVal);
    }
}



/* Performs advection:
    x := (i,j)
    output(x) = quantity(x - velocity * dt_h)

   dt_h is dt / h where
    dt := time increment
    h  := grid size
*/
__kernel void advect(__read_only image2d_t quantity,
                     __read_only image2d_t velocity,
                     __write_only image2d_t output,
                     const float dt_h)
{
    const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                              CLK_ADDRESS_CLAMP           |
                              CLK_FILTER_LINEAR;
    // NOTE: It is unclear whether linear interpolation is a good idea.
    // Try experimenting.

    float2 coords = (float2) (get_global_id(0), get_global_id(1));
    int2 icoords = (int2) (get_global_id(0), get_global_id(1));

    if (coords.x < get_image_width(output) && coords.y < get_image_height(output))
    {
        float2 vel = read_imagef(velocity, sampler, coords).xy;
        float2 offset = -vel * dt_h;

        float4 prev_quant = read_imagef(quantity, sampler, coords + (float2)(0.5, 0.5) + offset);

        write_imagef(output, icoords, prev_quant);
    }
}


__kernel void divergence(__read_only image2d_t field,
                         __write_only image2d_t output,
                         const float hInv)
{
    const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                              CLK_ADDRESS_CLAMP           |
                              CLK_FILTER_NEAREST;

    int2 coords = (int2) (get_global_id(0), get_global_id(1));

    if (coords.x < get_image_width(output) && coords.y < get_image_height(output))
    {
        float4 field_xp = read_imagef(field, sampler, (int2) (coords.x + 1, coords.y));
        float4 field_xm = read_imagef(field, sampler, (int2) (coords.x - 1, coords.y));
        float4 field_yp = read_imagef(field, sampler, (int2) (coords.x, coords.y + 1));
        float4 field_ym = read_imagef(field, sampler, (int2) (coords.x, coords.y - 1));

        write_imagef(output, coords, (float4) (((field_xp.x - field_xm.x) + (field_yp.y - field_ym.y)) * hInv, 0, 0, 0));
    }
}


__kernel void gradient(__read_only image2d_t field,
                       __write_only image2d_t output,
                       const float hInv)
{
    const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                              CLK_ADDRESS_CLAMP           |
                              CLK_FILTER_NEAREST;

    int2 coords = (int2) (get_global_id(0), get_global_id(1));

    if (coords.x < get_image_width(output) && coords.y < get_image_height(output))
    {
        float field_xp = read_imagef(field, sampler, (int2) (coords.x + 1, coords.y)).x;
        float field_xm = read_imagef(field, sampler, (int2) (coords.x - 1, coords.y)).x;
        float field_yp = read_imagef(field, sampler, (int2) (coords.x, coords.y + 1)).x;
        float field_ym = read_imagef(field, sampler, (int2) (coords.x, coords.y - 1)).x;

        float dx = (field_xp - field_xm) * hInv;
        float dy = (field_yp - field_ym) * hInv;

        write_imagef(output, coords, (float4) (dx, dy, 0, 0));
    }
}


__kernel void addScaled(__read_only image2d_t term1,
                        __read_only image2d_t term2,
                        const float multiplier,
                        __write_only image2d_t sum)
{
    const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                              CLK_ADDRESS_CLAMP           |
                              CLK_FILTER_NEAREST;

    int2 coords = (int2) (get_global_id(0), get_global_id(1));

    if (coords.x < get_image_width(sum) && coords.y < get_image_height(sum))
        write_imagef(sum, coords, read_imagef(term1, sampler, coords)
                                + read_imagef(term2, sampler, coords) * multiplier);
}


/* Sets the boundary value to the negation of its inner neighbor.

    | p1 | p2 ...

    p1 <- -p2
*/
__kernel void velocityBoundary(__read_only image2d_t img,
                               __write_only image2d_t out)
{
    const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                              CLK_ADDRESS_CLAMP           |
                              CLK_FILTER_NEAREST;

    int2 coords = (int2) (get_global_id(0), get_global_id(1));

    if (coords.x == 0)
        write_imagef(out, coords, -read_imagef(img, sampler, (int2) (1, coords.y)));
    else if (coords.x == get_image_width(out) - 1)
        write_imagef(out, coords, -read_imagef(img, sampler, (int2) (coords.x - 1, coords.y)));
    else if (coords.y == 0)
        write_imagef(out, coords, -read_imagef(img, sampler, (int2) (coords.x, 1)));
    else if (coords.y == get_image_height(out) - 1)
        write_imagef(out, coords, -read_imagef(img, sampler, (int2) (coords.x, coords.y - 1)));
    else if (coords.x < get_image_width(out) && coords.y < get_image_height(out))
        write_imagef(out, coords, read_imagef(img, sampler, coords));
}

/* Sets the boundary value to the value of its inner neighbor.

    | p1 | p2 ...

    p1 <- p2
*/
__kernel void pressureBoundary(__read_only image2d_t img,
                               __write_only image2d_t out)
{
   const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                             CLK_ADDRESS_CLAMP           |
                             CLK_FILTER_NEAREST;

   int2 coords = (int2) (get_global_id(0), get_global_id(1));

   if (coords.x == 0)
       write_imagef(out, coords, read_imagef(img, sampler, (int2) (/*coords.x + */1, coords.y)));
   else if (coords.x == get_image_width(out) - 1)
       write_imagef(out, coords, read_imagef(img, sampler, (int2) (coords.x - 1, coords.y)));
   else if (coords.y == 0)
       write_imagef(out, coords, read_imagef(img, sampler, (int2) (coords.x, /*coords.y + */1)));
   else if (coords.y == get_image_height(out) - 1)
       write_imagef(out, coords, read_imagef(img, sampler, (int2) (coords.x, coords.y - 1)));
   else if (coords.x < get_image_width(out) && coords.y < get_image_height(out))
       write_imagef(out, coords, read_imagef(img, sampler, coords));
}

__kernel void updateWind(__read_only image2d_t windVelocity,
                         __read_only image2d_t forces,
                         __write_only image2d_t output,
                         const float hinv, // 1 / grid size
                         const float dt,
                         const float viscosity)
{
    // TODO: I need to consider the pressure term.
    const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                              CLK_ADDRESS_CLAMP           |
                              CLK_FILTER_NEAREST;

    // Let L denote the Laplacian, and Du denote the directional derivative
    // in the direction u.
    // u' = -(Du)u + vLu + f
    int2 coords = (int2) (get_global_id(0), get_global_id(1));

    if (coords.x < get_image_width(output) && coords.y < get_image_height(output))
    {
    //    float2 f = read_imagef(forces, sampler, coords).xy;
        int width = get_image_width(windVelocity);
        int height = get_image_height(windVelocity);

        float2 f = (float)(0, 0); // TODO: Get rid of these lines.
        if (coords.x > width/2 && coords.x < (width/2 + 30) && coords.y == 0)
            f = (float2)(0, 1);
        if (coords.x == 0)
            f = (float2)(.7 / (1 + (abs(coords.y - height/4)) / 32.0), 0);
        if (coords.x == width - 1)
            f = (float2)(-.6 / (1 + (abs(coords.y - 3*height/4)) / 32.0), -0.6 / (1 + (abs(coords.y - 3*height/4)) / 32.0));

        float2 u = read_imagef(windVelocity, sampler, coords).xy;

        float2 u_ym = read_imagef(windVelocity, sampler, coords + (int2)(0, -1)).xy;
        float2 u_yp = read_imagef(windVelocity, sampler, coords + (int2)(0,  1)).xy;
        float2 u_xm = read_imagef(windVelocity, sampler, coords + (int2)(-1, 0)).xy;
        float2 u_xp = read_imagef(windVelocity, sampler, coords + (int2)( 1, 0)).xy;

        float2 u_x = (u_xp - u_xm) * hinv * 0.5f;
        float2 u_y = (u_yp - u_ym) * hinv * 0.5f;

        float hinv2 = hinv * hinv;
        float2 u_xx = (u_xp - 2 * u + u_xm) * hinv2;
        float2 u_yy = (u_yp - 2 * u + u_ym) * hinv2;

    //     u + u'h + u''h^2 / 2 + ...

        float2 du = (-(u.x * u_x + u.y * u_y)
                      + viscosity * (u_xx + u_yy)
                      + f) * dt;
        float2 newWind = u + du;
        float4 outVal = (float4)(newWind.x, newWind.y, 0, 0);
//        float4 outVal = (float4)(u_ym.xy, 0, 0);

        write_imagef(output, coords, outVal);
    }
}


__kernel void zeroTexture(__write_only image2d_t output)
{
    int2 coords = (int2) (get_global_id(0), get_global_id(1));
    if (coords.x == get_image_width(output) / 2 && coords.y == get_image_height(output) / 2)
        write_imagef(output, coords, (float4) (0.0001, 0, 0, 0));
}

// Adjusts grass blades in response to wind.
// The first two buffers will be both read and written.
__kernel void reactToWind(__global float2 *grassWindPositions,
                          __global float2 *grassWindVelocities,
                          __global float2 *grassNormalizedOffsets,
                          __read_only image2d_t windStrengthImage,
                          const unsigned int numBlades,
                          const float dt)
{
    unsigned int bladeIdx = get_global_id(0);

    const sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE  |
                              CLK_ADDRESS_CLAMP           |
                              CLK_FILTER_LINEAR;

    if (bladeIdx < numBlades)
    {
//        float2 windStrength = grassWindStrength[bladeIdx];
        float2 windStrength = read_imagef(windStrengthImage, sampler, grassNormalizedOffsets[bladeIdx]).xy;

        // For now, the neutral position will be equal to the wind strength.
        float wndMag = max(fast_length(windStrength), 1.0f);
        float2 neutralPosition = windStrength / wndMag;

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
