#include "Common.h"

std::wstring std::to_wstring(const string& str)
{
	std::wstring wstr;
	size_t size;
	wstr.resize(str.length());
	mbstowcs_s(&size, &wstr[0], wstr.size() + 1, str.c_str(), str.size());
	return wstr;
}

std::vector<std::string> std::to_string(const vector<wstring>& array_wstring)
{
	std::vector<std::string> array_string{};
	array_string.reserve(array_wstring.size());
	for (const std::wstring& wstr : array_wstring)
	{
		array_string.push_back(std::to_string(wstr));
	}
	return array_string;
}

std::string std::to_string(const wstring& wstr)
{
	std::string str;
	size_t size;
	str.resize(wstr.length());
	wcstombs_s(&size, &str[0], str.size() + 1, wstr.c_str(), wstr.size());
	return str;
}

std::vector<std::wstring> std::to_wstring(const vector<string>& array_string)
{
	std::vector<std::wstring> array_wstring{};
	array_wstring.reserve(array_string.size());
	for (const std::string& str : array_string)
	{
		array_wstring.push_back(std::to_wstring(str));
	}
	return array_wstring;
}
