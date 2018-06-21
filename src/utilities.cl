

__kernel void zeroInitialize(__write_only image2d_t img, int width, int height)
{
    int2 coords = (int2) (get_global_id(0), get_global_id(1));

    if (coords.x < width && coords.y < height)
        write_imagef(img, coords, (float4) (0,0,0,0));
}
