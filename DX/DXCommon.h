#pragma once

#include "../core/Common.h"

#include <d3dx12/d3dx12_state_object.h>
#include <d3d12.h> // D3D12
#include <dxgi1_6.h> // DXGI

#include <dxcapi.h> // DXC compiler

// Agility SDK needs to be included in main.cpp
#define AGILITY_SDK_DECLARE() extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 716; } \
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

static const uint32 g_backbuffer_count = 3u;

#if defined(_DEBUG)
#define NAME_DX_OBJECT(object, name) object->SetName(std::to_wstring(name).c_str()) >> CHK
#define NAME_DXGI_OBJECT(object, name) object->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(char) * COUNT(name), name) >> CHK;
#else
#define NAME_DX_OBJECT(object, name)
#define NAME_DXGI_OBJECT(object, name)
#endif

std::string RemapHResult(const HRESULT& hr);

struct CheckToken {};
extern CheckToken CHK;
struct HRSourceLocation
{
	HRSourceLocation(const HRESULT hr, std::source_location loc = std::source_location::current());

	HRESULT m_hr;
	std::source_location m_source_location;
};

void operator>>(HRSourceLocation hrSourceLocation, CheckToken);

inline int64 ToKB(int64 bytes) { return bytes >> 10; }
inline int64 ToMB(int64 bytes) { return bytes >> 20; }
inline int64 ToGB(int64 bytes) { return bytes >> 30; }

inline int64 FromKB(int64 KB) { return KB << 10; }
inline int64 FromMB(int64 MB) { return MB << 20; }
inline int64 FromGB(int64 GB) { return GB << 30; }


D3D12_SHADER_BYTECODE BlobToByteCode(ComPtr<IDxcBlob> blob);
