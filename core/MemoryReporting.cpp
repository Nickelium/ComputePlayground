#include "MemoryReporting.h"

// Only displays in OutDebugConsole when used with a debugger
void MemoryTrack()
{
	#if defined(MEMORY_REPORTING_ENABLE)
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	#endif
}