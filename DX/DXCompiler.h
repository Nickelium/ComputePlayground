#pragma once
#include "../core/Common.h"
#include <dxcapi.h> // DXC compiler

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
	DXCompiler(const std::string& directory);
	void Compile(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<IDxcBlob>* out_shader_blob, const std::string& shader_path, const ShaderType shader_type) const;
private:
	void Init(const std::string& directory);
	
	bool m_debug;
	std::string m_directory;

	Microsoft::WRL::ComPtr<IDxcUtils> m_utils;
	Microsoft::WRL::ComPtr<IDxcCompiler3> m_compiler;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> m_include_handler;
};