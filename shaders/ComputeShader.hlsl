#include "Common.hlsl"
// TODO shader assert readback
// TODO shader printf readback
//https://therealmjp.github.io/posts/hlsl-printf/
// TODO shader printf GPU
// TODO shader unit test readback
// TODO shader debug draw
//https://www.gijskaerts.com/wordpress/?p=190

struct MyCBuffer
{
	int2 resolution;
	float iTime;
	int iFrame;
	int bindless_index;
};

ConstantBuffer<MyCBuffer> m_cbuffer : register(b0);

//#define CHEAP_STAR
#define JULIA

float cheap_star(float2 uv, float anim)
{
	uv = abs(uv);
	float2 pos = min(uv.xy/uv.yx, anim);
	float p = (2.0 - pos.x - pos.y);
	return (2.0+p*(p*p-1.5)) / (uv.x+uv.y);      
}

float3 animate_star(float2 uv, float iTime)
{
	uv += -0.5f;
	uv *= 2.0 * ( cos(iTime * 2.0) -2.5); // scale
	float anim = sin(iTime * 12.0) * 0.1 + 1.0;  // anim between 0.9 - 1.1 
	return float3(cheap_star(uv, anim) * float3(0.35,0.2,0.15));
}

#define ITR 100
#define PI 3.1415926

struct Complex
{
	float re;
	float im;

	Complex operator+(Complex c1)
	{
		Complex c;
		c.re = re + c1.re;
		c.im = im + c1.im;
		return c;
	}

	Complex operator*(Complex c1)
	{
		Complex c;
		c.re = re * c1.re - im * c1.im;
		c.im = re * c1.im + im * c1.re;
		return c;
	}

	float Length()
	{
		return sqrt(re * re + im * im);
	}

	float LengthSquared()
	{
		return re * re + im * im;
	}
};

float julia(Complex z0, Complex c, int N)
{
	Complex zN = z0;
	int i = 0;
	while (zN.LengthSquared() < 4.0f && i < N)
	{
		zN = zN * zN + c;
		++i;
	}
	return i - log2(max(1.0f, log2(zN.Length())));
}

uint WangHash(uint seed)
{
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
	return seed;
}

uint Xorshift(uint seed)
{
	// Xorshift algorithm from George Marsaglia's paper
	seed ^= (seed << 13);
	seed ^= (seed >> 17);
	seed ^= (seed << 5);
	return seed;
}

float GenerateRandomNumber(inout uint seed)
{
	seed = WangHash(seed);
	return float(Xorshift(seed)) * (1.f / 4294967296.f);
}

/**
	* Generate three components of white noise in image-space.
*/
float3 GetWhiteNoise(uint2 position, uint width, uint frame, float scale)
{
	// Generate a unique seed based on:
	// space - this thread's (x, y) position in the image
	// time  - the current frame number
	uint seed = ((position.y * width) + position.x) * frame;

	// Generate three uniformly distributed random values in the range [0, 1]
	float3 rnd;
	rnd.x = GenerateRandomNumber(seed);
	rnd.y = GenerateRandomNumber(seed);
	rnd.z = GenerateRandomNumber(seed);

	// D3D rounds when converting from FLOAT to UNORM
	// Shift the random values from [0, 1] to [-0.5, 0.5]
	rnd -= 0.5f;

	// Scale the noise magnitude, values are in the range [-scale/2, scale/2]
	// The scale should be determined by the precision (and therefore quantization amount) of the target image's format
	return (rnd * scale);
}


[RootSignature(ROOTFLAGS_DEFAULT ", RootConstants(num32BitConstants=5, b0)")]
[numthreads(8, 8, 1)]
void main
(
	const uint3 inGroupThreadID : SV_GroupThreadID,
	const uint3 inGroupID : SV_GroupID,
	const uint3 inDispatchThreadID : SV_DispatchThreadID,
	const uint inGroupIndex : SV_GroupIndex
)
{
	float2 uv = (inDispatchThreadID.xy + 0.5f) / float2(m_cbuffer.resolution.x, m_cbuffer.resolution.y);
	RWTexture2D<float4> m_uav = ResourceDescriptorHeap[m_cbuffer.bindless_index];
	float3 out_color;
#if defined(CHEAP_STAR)
	out_color = animate_star(uv, m_cbuffer.iTime);
	out_color = sRGBToLinear(out_color);
#elif defined(JULIA)
	Complex z0;
	float maxFrame = 1000.0f;
	float scale = lerp(0.75, 1.5, (m_cbuffer.iFrame) / maxFrame);
	z0.re = (uv.x * 2 - 1) * scale;
	z0.im = (uv.y * 2 - 1) * scale;
	Complex c;
	c.re = (cos(m_cbuffer.iTime * 0.12) - 1) * 0.125;
	c.im = sin(m_cbuffer.iTime * 0.15);
//	c.re = 0.285;
//	c.im = 0;
	int N = 50;
	float j = julia(z0, c, N);
	out_color = 0.5f + 0.5 * cos( 3.0 + j *0.15 + float3(0.0,0.6,1.0));

	float noiseScale = 1.0f / 256.0f;
	float3 noise = GetWhiteNoise(uint2(inDispatchThreadID.xy + 0.5f), m_cbuffer.resolution.x, m_cbuffer.iFrame, noiseScale);
	out_color += noise;
	//out_color = sRGBToLinear(out_color);
#else
	out_color = float3((sin(m_cbuffer.iTime * 1.5) + 1) * 0.25 + uv.x, (cos(m_cbuffer.iTime * 2) + 1) * 0.125 + uv.y, 0);
	out_color = float3(uv, 0.0f);
	out_color = sRGBToLinear(out_color);
#endif
	// Presentation to display
	m_uav[inDispatchThreadID.xy] = float4(LinearTosRGB(out_color), 1.0f);
}