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
	HRSourceLocation(HRESULT hr, std::source_location loc = std::source_location::current())
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
		int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
		std::wstring wstr(count, 0);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstr[0], count);
		return wstr;
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