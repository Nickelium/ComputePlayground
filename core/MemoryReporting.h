#pragma once

#if defined(_DEBUG)
#define MEMORY_REPORTING_ENABLE
#endif

#if defined(MEMORY_REPORTING_ENABLE)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
// Display file and line number on memory leak
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

void MemoryTrack();