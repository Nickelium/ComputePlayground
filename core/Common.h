#pragma once

// STL
#include <vector>
#include <string>
#include <iterator>
#include <vector>
#include <source_location>
// ComPtr
#define NOMINMAX
#include <wrl/client.h>
// Avoid long namespace for Microsoft::WRL::ComPtr<T>
// using namespace Microsoft::WRL;

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

// Additional string std conversions
namespace std
{
	std::wstring to_wstring(const std::string& str);
	std::string to_string(const std::wstring& wstr);
	std::vector<std::wstring> to_wstring(const std::vector<std::string>& array_string);
	std::vector<std::string> to_string(const std::vector<std::wstring>& array_wstring);
}

#include "Types.h"
#include "Logger.h"
#include "MemoryReporting.h"
