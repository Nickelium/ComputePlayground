#pragma once

float Reinhard(float x)
{
    return x / (x + 1.0f);
}

float3 Reinhard(float3 v)
{
    return 
        float3
        (
            Reinhard(v.r),
            Reinhard(v.g),
            Reinhard(v.b)
        );
}