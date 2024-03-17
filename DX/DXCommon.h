#pragma once

#include <d3d12.h> // D3D12
//#include <d3d12sdklayers.h> // already included in d3d12.h
#include <dxgi1_6.h> // DXGI

// Agility SDK needs to be included in main.cpp
#define AGILITY_SDK_DECLARE() extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 611; } \
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

static const uint32 g_backbuffer_count = 3u;