#pragma once

#include "MemoryReporting.h"


// STL
#include <vector>
#include <string>
#include <iterator>
#include <vector>
#include <algorithm>
#include <map>
#include <source_location>
// ComPtr
#define NOMINMAX
#include <wrl/client.h>
// Avoid long namespace for ComPtr<T>
using namespace Microsoft::WRL;

#include "Types.h"
#include "Logger.h"

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

inline uint32 Align(uint32 x, uint32 align)
{
	return (x + align - 1) & ~(align - 1);
}

inline uint32 DivideRoundUp(uint32 numerator, uint32 denominator)
{
	return (numerator + denominator - 1) / denominator;
}

#define HLSLPP_FEATURE_TRANSFORM
#include <hlsl++.h>
using namespace hlslpp;