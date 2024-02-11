#pragma once
#include "../Common.h"

struct IDxcUtils;
struct IDxcBlob;
struct IDxcCompiler3;
struct IDxcIncludeHandler;
struct ID3D12Device;

enum class ShaderType
{
	VERTEX_SHADER = 0,
	PIXEL_SHADER,
	COMPUTE_SHADER,
	NUMBER_SHADER_TYPES,
};

class DXCompiler
{
public:
	void Init(const std::wstring& directory);
	void Compile(ComPtr<ID3D12Device> device, ComPtr<IDxcBlob>* out_shader_blob, const std::wstring& shader_path, const ShaderType shader_type) const;
private:
	bool m_debug;
	std::wstring m_directory;

	ComPtr<IDxcUtils> m_utils;
	ComPtr<IDxcCompiler3> m_compiler;
	ComPtr<IDxcIncludeHandler> m_include_handler;
};