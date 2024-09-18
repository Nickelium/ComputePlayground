#include "DXCommon.h"
// Note: Requires C++23
//#include <stacktrace> 

CheckToken CHK = CheckToken{};

std::string RemapHResult(const HRESULT& hr)
{
	return std::system_category().message(hr);
}

void operator>>(const HRSourceLocation hr_source_location, const CheckToken chk_token)
{
#if defined(_DEBUG)
	if (hr_source_location.m_hr != S_OK)
	{
		std::string error_code_string = RemapHResult(hr_source_location.m_hr);
		std::string line_string =
			"Error at " +
			std::string(hr_source_location.m_source_location.function_name()) + " line " +
			std::to_string(hr_source_location.m_source_location.line());

		//auto trace = std::stacktrace::current();
		//LogError(std::to_string(trace));
		LogError(line_string);
		LogError("Error Code " + std::to_string(hr_source_location.m_hr) + ": " + error_code_string);
		ASSERT(false);
	}
#else
	UNUSED(hr_source_location);
#endif
	UNUSED(chk_token);
}

HRSourceLocation::HRSourceLocation(const HRESULT hr, std::source_location loc /*= std::source_location::current()*/) : m_hr(hr),
m_source_location(loc)
{

}
