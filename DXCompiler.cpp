#include "DXCompiler.h"
#include "dxcapi.h" // DXC compiler

void DXCompiler::Init(bool debug)
{
	m_debug = debug;
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)) >> CHK;
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)) >> CHK;
}

void DXCompiler::Compile(ComPtr<IDxcBlob>* outShaderBlob, std::wstring shaderPath, ShaderType shaderType)
{
	ComPtr<IDxcBlobEncoding> shaderSource{};
	utils->LoadFile(shaderPath.c_str(), nullptr, &shaderSource) >> CHK;
	DxcBuffer sourceBuffer{};
	sourceBuffer.Ptr = shaderSource->GetBufferPointer();
	sourceBuffer.Size = shaderSource->GetBufferSize();

	std::vector<LPCWSTR> compileArguments;
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

	ComPtr<IDxcResult> compileResult;
	compiler->Compile(&sourceBuffer, compileArguments.data(), (UINT32)compileArguments.size(), nullptr, IID_PPV_ARGS(&compileResult)) >> CHK;

	ComPtr<IDxcBlobUtf8> pErrors;
	compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr) >> CHK;
	if (pErrors && pErrors->GetStringLength() > 0)
	{
		printf("%s", (char*)pErrors->GetBufferPointer());
	}
	HRESULT HR;
	compileResult->GetStatus(&HR);
	HR >> CHK;

	compileResult->GetOutput(DXC_OUT_OBJECT, __uuidof(IDxcBlob), (void**)outShaderBlob, nullptr) >> CHK;
}

