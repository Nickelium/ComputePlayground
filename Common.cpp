#include "Common.h"

CheckToken CHK = CheckToken{};

void operator>>(const HRSourceLocation hrSourceLocation, const CheckToken chk_token)
{
#if defined(_DEBUG)
	if (hrSourceLocation.m_hr != S_OK)
	{
		// TODO HResult into string
		std::string stringOuput =
			"Error: " +
			std::to_string(hrSourceLocation.m_hr) + " at " +
			std::string(hrSourceLocation.m_sourceLocation.function_name()) + " line " +
			std::to_string(hrSourceLocation.m_sourceLocation.line()) + "\n";
		OutputDebugStringW(std::to_wstring(stringOuput).c_str());
		printf("%s", stringOuput.c_str());
		ASSERT(false);
	}
#else
	UNUSED(hrSourceLocation);
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

void AssertHook()
{
#if defined(_DEBUG)
	// TODO assert hook stop hooking memory tracker
	// Hook up callback on assert
	_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, CrtDbgHook);
#endif
}
