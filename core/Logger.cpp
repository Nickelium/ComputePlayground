#include "Logger.h"
#include "Common.h"

#if defined(_DEBUG)
#define LOG_ENABLE
#endif

#if defined(LOG_ENABLE)
// Added _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING globally to silence spdlog fmt issue with latest MSVC
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
// Release version 1.13
#include "spdlog/spdlog.h"
#endif

#if defined(LOG_ENABLE)
namespace
{
	enum class LogLevel
	{
		Trace = 0,
		Error
	};

	// Only available when connected to a debugger
	void PrintOutput(const std::string& string)
	{
		std::string string_ln = string + "\n";
		OutputDebugStringW(std::to_wstring(string_ln).c_str());
	}

	void PrintConsole(const std::string& string, const LogLevel& log_level)
	{
		std::string string_ln = string + "\n";
		if(log_level == LogLevel::Trace)
			spdlog::info(string_ln);
		else
			spdlog::error(string_ln);
	}

	void Log(const std::string& string, const LogLevel& log_level)
	{
		PrintConsole(string, log_level);
		if(log_level == LogLevel::Error)
			PrintOutput(string);
	}
}
#endif

// API
void LogTrace(const std::string& string)
{
#if defined(LOG_ENABLE)
	Log(string, LogLevel::Trace);
#else
	UNUSED(string);
#endif
}

void LogError(const std::string& string)
{
#if defined(LOG_ENABLE)
	Log(string, LogLevel::Error);
#else
	UNUSED(string);
#endif
}