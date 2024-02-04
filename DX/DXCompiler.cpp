#include "DXCompiler.h"
#include "dxcapi.h" // DXC compiler

void DXCompiler::Init(const bool debug, const std::wstring& directory)
{
	m_debug = debug;
	m_directory = directory;
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_utils)) >> CHK;
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)) >> CHK;
	m_utils->CreateDefaultIncludeHandler(&m_include_handler);
}

void DXCompiler::Compile(ComPtr<IDxcBlob>* outShaderBlob, const std::wstring& shaderFile, const ShaderType shaderType) const
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
	std::wstring shaderModel{};
	switch (shaderType)
	{
	case ShaderType::VERTEX_SHADER:
		shaderModel = L"vs_6_6";
		break;
	case ShaderType::PIXEL_SHADER:
		shaderModel = L"ps_6_6";
		break;
	case ShaderType::COMPUTE_SHADER:
		shaderModel = L"cs_6_6";
		break;

	}
	compileArguments.push_back(shaderModel.c_str());
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

