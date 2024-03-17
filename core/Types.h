#pragma once

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

using float2 = float32[2];
using float3 = float32[3]; 
using float4 = float32[4];