#pragma once
#include "Common.h"

struct IDxcUtils;
struct IDxcBlob;
struct IDxcCompiler3;

enum class ShaderType
{
	VERTEX_SHADER,
	PIXEL_SHADER,
	COMPUTE_SHADER,
};

class DXCompiler
{
public:
	void Init(bool debug);
	void Compile(ComPtr<IDxcBlob>* outShaderBlob, std::wstring shaderPath, ShaderType shaderType);
private:
	bool m_debug;

	ComPtr<IDxcUtils> utils;
	ComPtr<IDxcCompiler3> compiler;
};