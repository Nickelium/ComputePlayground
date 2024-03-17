#pragma once

#include <string>
#include <format>
#include <string_view>

void LogTrace(const std::string& string);
void LogError(const std::string& string);

template<typename ... Args>
void LogTrace(const std::string& string, Args && ... args)
{
	std::string_view string_view = string;
	// vformat to allow non-compile time arguments
	std::string formatted_string = std::vformat(string_view, std::make_format_args(args...));
	LogTrace(formatted_string);
}

template<typename ... Args>
void LogError(const std::string& string, Args && ... args)
{
	std::string_view string_view = string;
	// vformat to allow non-compile time arguments
	std::string formatted_string = std::vformat(string_view, std::make_format_args(args...));
	LogError(formatted_string);
}