#pragma once
#include "../Common.h"

struct IDxcUtils;
struct IDxcBlob;
struct IDxcCompiler3;
struct IDxcIncludeHandler;

enum class ShaderType
{
	VERTEX_SHADER,
	PIXEL_SHADER,
	COMPUTE_SHADER,
};

class DXCompiler
{
public:
	void Init(const bool debug, const std::wstring& directory);
	void Compile(ComPtr<IDxcBlob>* out_shader_blob, const std::wstring& shader_path, const ShaderType shader_type) const;
private:
	bool m_debug;
	std::wstring m_directory;

	ComPtr<IDxcUtils> m_utils;
	ComPtr<IDxcCompiler3> m_compiler;
	ComPtr<IDxcIncludeHandler> m_include_handler;
};