#pragma once

#include <vector>
#include <cassert>
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

#if defined(_DEBUG)
void operator>>(HRSourceLocation hrSourceLocation, CheckToken);
#else
void operator>>(HRSourceLocation, CheckToken) {}
#endif

int __cdecl CrtDbgHook(int nReportType, char* szMsg, int* pnRet);

#define DISABLE_OPTIMISATIONS() __pragma( optimize( "", off ) )
#define ENABLE_OPTIMISATIONS() __pragma( optimize( "", on ) )