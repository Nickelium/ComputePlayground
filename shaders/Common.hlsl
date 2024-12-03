#pragma once

// CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED for bindless
#define ROOTFLAGS_DEFAULT "RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | DENY_HULL_SHADER_ROOT_ACCESS | DENY_DOMAIN_SHADER_ROOT_ACCESS | DENY_GEOMETRY_SHADER_ROOT_ACCESS)"

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

float LinearTosRGB(float x)
{
	if(x <= 0.0031308f)
		return 12.92f * x;
	else
		return 1.055f * pow(x, 1.0f / 2.4f) - 0.055f;
}

float3 LinearTosRGB(float3 c)
{
	return float3(
		LinearTosRGB(c.r), 
		LinearTosRGB(c.g), 
		LinearTosRGB(c.b));
}

float sRGBToLinear(float x)
{
	if(x <= 0.04045f)
		return x / 12.92f;
	else
		return pow((x + 0.055f) / 1.055f, 2.4f);
}

float3 sRGBToLinear(float3 c)
{
	return float3(
		sRGBToLinear(c.r), 
		sRGBToLinear(c.g), 
		sRGBToLinear(c.b));
}

float LinearTosRGBApprox(float x)
{
	return pow(x, 1.0f / 2.2f);
}

float3 LinearTosRGBApprox(float3 c)
{
	return float3(
		LinearTosRGBApprox(c.r), 
		LinearTosRGBApprox(c.g), 
		LinearTosRGBApprox(c.b));
}

float sRGBToLinearApprox(float x)
{
	return pow(x, 2.2f);
}

float3 sRGBToLinearApprox(float3 c)
{
	return float3(
		sRGBToLinearApprox(c.r), 
		sRGBToLinearApprox(c.g), 
		sRGBToLinearApprox(c.b));
}