#include "Common.h"

CheckToken CHK = CheckToken{};

void operator>>(HRSourceLocation hrSourceLocation, CheckToken)
{
#if defined(_DEBUG)
	if (hrSourceLocation.m_hr != S_OK)
	{
		// TODO HResult into string
		std::wstring stringOuput =
			L"Error: " +
			std::to_wstring(hrSourceLocation.m_hr) + L" at " +
			std::to_wstring(hrSourceLocation.m_sourceLocation.function_name()) + L" line " +
			std::to_wstring(hrSourceLocation.m_sourceLocation.line()) + L"\n";
		OutputDebugStringW(stringOuput.c_str());
		printf("%ls", stringOuput.c_str());
		ASSERT(false);
	}

#else
	UNUSED(hrSourceLocation);
#endif
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
