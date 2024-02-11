#include "DXCompiler.h"
#include "dxcapi.h" // DXC compiler
#include "DXContext.h"
#include "DXQuery.h"

static const std::pair<ShaderType, std::string> g_shader_type_map_string[] =
{
	{ShaderType::VERTEX_SHADER, "vs"},
	{ShaderType::PIXEL_SHADER, "ps"},
	{ShaderType::COMPUTE_SHADER, "cs"},
};

std::string GetShaderTypeString(const ShaderType& shader_type)
{
	return g_shader_type_map_string[static_cast<int32>(shader_type)].second;
}

void DXCompiler::Init(const std::wstring& directory)
{
#if defined(_DEBUG)
	m_debug = true;
#else
	m_debug = false;
#endif
	m_directory = directory;
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_utils)) >> CHK;
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)) >> CHK;
	m_utils->CreateDefaultIncludeHandler(&m_include_handler);
}

void DXCompiler::Compile(ComPtr<ID3D12Device> device, ComPtr<IDxcBlob>* outShaderBlob, const std::wstring& shaderFile, const ShaderType shaderType) const
{
	ComPtr<IDxcBlobEncoding> shaderSource{};
	const std::wstring shaderFullPath = m_directory + std::wstring(L"\\") + shaderFile;
	m_utils->LoadFile(shaderFullPath.c_str(), nullptr, &shaderSource) >> CHK;
	DxcBuffer sourceBuffer{};
	sourceBuffer.Ptr = shaderSource->GetBufferPointer();
	sourceBuffer.Size = shaderSource->GetBufferSize();

	std::vector<LPCWSTR> compileArguments{};
	compileArguments.push_back(L"-E "); // Entry point 
	compileArguments.push_back(L"main");
	compileArguments.push_back(L"-T"); // Target profile 
	std::wstring shader_type_model_string = std::to_wstring(GetShaderTypeString(shaderType));

	const D3D_SHADER_MODEL shader_model = GetMaxShaderModel(device);
	const std::string& shader_model_string = GetShaderModelString(shader_model);
	shader_type_model_string += L"_" + std::to_wstring(shader_model_string);

	compileArguments.push_back(shader_type_model_string.c_str());
	if (m_debug)
	{
		compileArguments.push_back(L"-Od"); // Disable Optimizations
		compileArguments.push_back(L"-Zi"); // Debug symbols
	}
	else
	{
		//compileArguments.push_back(L"-O1"); // Min Optimizations
		//compileArguments.push_back(L"-O2"); // Mid Optimizations
		compileArguments.push_back(L"-O3"); // Max Optimizations
	}
	//compileArguments.push_back(L"-Fd"); // PDB options followed by pdbpath
	compileArguments.push_back(L"-HV 2021"); // HLSL 2021
	compileArguments.push_back(L"-WX"); // Warnings are errors
	std::wstring include_string_argument = L"-I " + m_directory;
	compileArguments.push_back(include_string_argument.c_str());

	ComPtr<IDxcResult> compileResult{};
	m_compiler->Compile(&sourceBuffer, compileArguments.data(), (UINT32)compileArguments.size(), m_include_handler.Get(), IID_PPV_ARGS(&compileResult)) >> CHK;

	ComPtr<IDxcBlobUtf8> pErrors{};
	compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr) >> CHK;
	if (pErrors && pErrors->GetStringLength() > 0)
	{
		printf("%s", (char*)pErrors->GetBufferPointer());
	}
	HRESULT HR{};
	compileResult->GetStatus(&HR) >> CHK;
	HR >> CHK;

	compileResult->GetOutput(DXC_OUT_OBJECT, __uuidof(IDxcBlob), (void**)outShaderBlob, nullptr) >> CHK;
}

