#include "Common.h"
#include <system_error>

// Note: Requires C++23
#include <stacktrace> 
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
		
		auto trace = std::stacktrace::current();
		LogError(std::to_string(trace));
		LogError(line_string);
		LogError("Error Code " + std::to_string(hr_source_location.m_hr) + ": "+ error_code_string);
		ASSERT(false);
	}
#else
	UNUSED(hr_source_location);
#endif
	UNUSED(chk_token);
}

int __cdecl CrtDbgHook(int nReportType, char* szMsg, int* pnRet)
{
	UNUSED(nReportType);
	UNUSED(szMsg);
	UNUSED(pnRet);
	__debugbreak();
	return TRUE;//Return true - Abort,Retry,Ignore dialog will *not* be displayed
}

void MemoryTrack()
{
#if defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
}

void MemoryDump()
{
#if defined(_DEBUG)
	_CrtDumpMemoryLeaks();
#endif
}

//void AssertHook()
//{
//#if defined(_DEBUG)
//	// TODO assert hook stop hooking memory tracker
//	// Hook up callback on assert
//	_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, CrtDbgHook);
//#endif
//}
