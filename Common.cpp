#include "Common.h"

CheckToken CHK = CheckToken{};

void operator>>(HRSourceLocation hrSourceLocation, CheckToken)
{
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
		assert(false);
	}
}

int __cdecl CrtDbgHook(int nReportType, char* szMsg, int* pnRet)
{
	__debugbreak();
	return TRUE;//Return true - Abort,Retry,Ignore dialog will *not* be displayed
}
