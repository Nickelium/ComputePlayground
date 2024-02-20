#pragma once

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
// Display file and line number on memory leak
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

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

struct CheckToken {};
extern CheckToken CHK;
struct HRSourceLocation 
{
	HRSourceLocation(const HRESULT hr, std::source_location loc = std::source_location::current())
		: m_hr(hr),
		m_sourceLocation(loc)
	{}
	HRESULT m_hr;
	std::source_location m_sourceLocation;
};

namespace std
{
	inline std::wstring to_wstring(const std::string& str)
	{
		std::wstring wstr;
		size_t size;
		wstr.resize(str.length());
		mbstowcs_s(&size, &wstr[0], wstr.size() + 1, str.c_str(), str.size());
		return wstr;
	}

	inline std::string to_string(const std::wstring& wstr)
	{
		std::string str;
		size_t size;
		str.resize(wstr.length());
		wcstombs_s(&size, &str[0], str.size() + 1, wstr.c_str(), wstr.size());
		return str;
	}

	inline std::vector<std::wstring> to_wstring(const std::vector<std::string>& array_string)
	{
		std::vector<std::wstring> array_wstring{};
		array_wstring.reserve(array_string.size());
		for (const std::string& str : array_string)
		{
			array_wstring.push_back(std::to_wstring(str));
		}
		return array_wstring;
	}

	inline std::vector<std::string> to_string(const std::vector<std::wstring>& array_wstring)
	{
		std::vector<std::string> array_string{};
		array_string.reserve(array_wstring.size());
		for (const std::wstring& wstr : array_wstring)
		{
			array_string.push_back(std::to_string(wstr));
		}
		return array_string;
	}
}

void operator>>(HRSourceLocation hrSourceLocation, CheckToken);

int __cdecl CrtDbgHook(int nReportType, char* szMsg, int* pnRet);

#define DISABLE_OPTIMISATIONS() __pragma( optimize( "", off ) )
#define ENABLE_OPTIMISATIONS() __pragma( optimize( "", on ) )
#define DEBUG_BREAK() __debugbreak()
// https://web.archive.org/web/20201129200055/http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert/
#define UNUSED(x) do { (void)sizeof(x); } while(false)
#if defined(_DEBUG)
#include <cassert>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x) UNUSED(x)
#endif

#define countof _countof

#if defined(_DEBUG)
#define NAME_DX_OBJECT(object, name) object->SetName(std::to_wstring(name).c_str()) >> CHK
#define NAME_DXGI_OBJECT(object, name) object->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(char) * countof(name), name) >> CHK;
#else
#define NAME_DX_OBJECT(object, name)
#define NAME_DXGI_OBJECT(object, name)
#endif

void MemoryTrack();
void MemoryDump();
void AssertHook();

// TODO remove this from non debug
enum class GRAPHICS_DEBUGGER_TYPE
{
	PIX,
	RENDERDOC,
	NONE,
};

// TODO split out this application logic
struct State
{
	bool m_capture;
};