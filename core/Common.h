#pragma once

#include "MemoryReporting.h"

#include <vector>
#include <string>
#include <iterator>
#include <vector>
#include <fstream>
#include <source_location>
#include <codecvt>
#include <locale>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl/client.h>
using namespace Microsoft::WRL;

#include "Types.h"

std::string RemapHResult(const HRESULT& hr);

struct CheckToken {};
extern CheckToken CHK;
struct HRSourceLocation 
{
	HRSourceLocation(const HRESULT hr, std::source_location loc = std::source_location::current())
		: m_hr(hr),
		m_source_location(loc)
	{}
	HRESULT m_hr;
	std::source_location m_source_location;
};

namespace std
{
	std::wstring to_wstring(const std::string& str);
	std::string to_string(const std::wstring& wstr);
	std::vector<std::wstring> to_wstring(const std::vector<std::string>& array_string);
	std::vector<std::string> to_string(const std::vector<std::wstring>& array_wstring);
}

void operator>>(HRSourceLocation hrSourceLocation, CheckToken);
#define DISABLE_OPTIMISATIONS() __pragma( optimize( "", off ) )
#define ENABLE_OPTIMISATIONS() __pragma( optimize( "", on ) )
#define DEBUG_BREAK() __debugbreak()
// https://web.archive.org/web/20201129200055/http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert/
#define UNUSED(x)  do { (void)sizeof(x); } while(0)
#if defined(_DEBUG)
#define ASSERT(x) do { if (!(x)) { DEBUG_BREAK(); } } while (0)
#else
#define ASSERT(x) UNUSED(x)
#endif

#define COUNT _countof

#if defined(_DEBUG)
#define NAME_DX_OBJECT(object, name) object->SetName(std::to_wstring(name).c_str()) >> CHK
#define NAME_DXGI_OBJECT(object, name) object->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(char) * COUNT(name), name) >> CHK;
#else
#define NAME_DX_OBJECT(object, name)
#define NAME_DXGI_OBJECT(object, name)
#endif

void MemoryTrackStart();

enum class GRAPHICS_DEBUGGER_TYPE
{
	PIX,
	RENDERDOC,
	NONE,
};

#include "Logger.h"