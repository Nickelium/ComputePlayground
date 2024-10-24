#pragma once
#include "../core/Common.h"
#include <dxcapi.h> // DXC compiler
#include "Shader.h"

struct IDxcUtils;
struct IDxcBlob;
struct IDxcCompiler3;
struct IDxcIncludeHandler;
struct ID3D12Device;


class DXCompiler
{
public:
	DXCompiler(const std::string& directory);
	Shader Compile(Microsoft::WRL::ComPtr<ID3D12Device> device, const ShaderDesc& shader_desc) const;
private:
	void Init(const std::string& directory);
	
	bool m_debug;
	std::string m_directory;

	Microsoft::WRL::ComPtr<IDxcUtils> m_utils;
	Microsoft::WRL::ComPtr<IDxcCompiler3> m_compiler;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> m_include_handler;
};