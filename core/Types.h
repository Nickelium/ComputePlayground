#pragma once

using uint8 = unsigned char;
using int8 = char;
using int32 = int; 
using uint32 = unsigned int;
static_assert(sizeof(int32) == 4);
static_assert(sizeof(uint32) == 4);
using int64 = int64_t;
using uint64 = uint64_t;
static_assert(sizeof(int64) == 8);
static_assert(sizeof(uint64) == 8);
using float32 = float;
using float64 = double;
static_assert(sizeof(float32) == 4);
static_assert(sizeof(float64) == 8);

template<typename T>
T MaxType();

template<typename T>
T MinType();

// Full specialization becomes a regular function, thus can have issue of multiple definitions across translation units
template<>
inline uint8 MaxType() { return (1 << 8) - 1; }

template<>
inline uint8 MinType() { return 0; }

template<>
inline int8 MaxType()
{
	int8 max_abs_value = static_cast<int8>(1 << 7);
	return +(max_abs_value - 1);
}

template<>
inline int8 MinType()
{
	int8 max_abs_value = static_cast < int8>(1 << 7);
	return -max_abs_value;
}

using int2 = int32[2];
using int3 = int32[3]; 
using int4 = int32[4];

using uint2 = uint32[2];
using uint3 = uint32[3]; 
using uint4 = uint32[4];

using float2 = float32[2];
using float3 = float32[3]; 
using float4 = float32[4];