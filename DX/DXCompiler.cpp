#include "DXCompiler.h"
#include "DXContext.h"
#include "DXQuery.h"

#if defined(_DEBUG)
#define DXC_COMPILER_DEBUG_ENABLE
#endif

static const std::pair<ShaderType, std::string> g_shader_type_map_string[] =
{
	{ShaderType::VERTEX_SHADER, "vs"},
	{ShaderType::PIXEL_SHADER, "ps"},
	{ShaderType::COMPUTE_SHADER, "cs"},
	{ShaderType::LIB_SHADER, "lib"},
};

std::string GetShaderTypeString(const ShaderType& shader_type)
{
	return g_shader_type_map_string[static_cast<int32>(shader_type)].second;
}

DXCompiler::DXCompiler(const std::string& directory)
{
	Init(directory);
}

void DXCompiler::Init(const std::string& directory)
{
	#if defined(DXC_COMPILER_DEBUG_ENABLE)
		m_debug = true;
	#else
		m_debug = false;
	#endif
	m_directory = directory;
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_utils)) >> CHK;
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)) >> CHK;
	m_utils->CreateDefaultIncludeHandler(&m_include_handler);
}

Shader DXCompiler::Compile(Microsoft::WRL::ComPtr<ID3D12Device> device, const ShaderDesc& shader_desc) const
{
	Microsoft::WRL::ComPtr<IDxcBlob> shader_blob;
	std::string shaderFile = shader_desc.m_file_name;
	ShaderType shaderType = shader_desc.m_type;
	std::string entry_point = shader_desc.m_entry_point_name;
	
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource{};
	const std::string shaderFullPath = m_directory + "\\" + shaderFile;
	m_utils->LoadFile(std::to_wstring(shaderFullPath).c_str(), nullptr, &shaderSource) >> CHK;
	DxcBuffer sourceBuffer{};
	sourceBuffer.Ptr = shaderSource->GetBufferPointer();
	sourceBuffer.Size = shaderSource->GetBufferSize();

	std::vector<std::string> compile_arguments{};
	compile_arguments.push_back("-E "); // Entry point name
	compile_arguments.push_back(entry_point);
	compile_arguments.push_back("-T"); // Target profile 
	std::string shader_type_model_string = GetShaderTypeString(shaderType);

	// Technically this needs to be supported by the DXC and not the device
	const D3D_SHADER_MODEL shader_model = GetMaxShaderModel(device);

	const std::string& shader_model_string = GetShaderModelString(shader_model);
	shader_type_model_string += "_" + shader_model_string;

	compile_arguments.push_back(shader_type_model_string.c_str());
	if (m_debug)
	{
		compile_arguments.push_back("-Od"); // Disable Optimizations
		compile_arguments.push_back("-Zi"); // Debug Informations/Symbols
	}
	else
	{
		//compile_arguments.push_back("-O0"); // Optimization Level 0
		//compile_arguments.push_back("-O1"); // Optimization Level 1
		//compile_arguments.push_back("-O2"); // Optimization Level 2
		compile_arguments.push_back("-O3"); // Optimization Level 3 (Default)
	}
	//compile_arguments.push_back("-Fd"); // PDB options followed by pdbpath
	compile_arguments.push_back("-HV 2021"); // HLSL 2021
	compile_arguments.push_back("-WX"); // 	Treat warnings as errors
	std::string include_string_argument = "-I " + m_directory;
	compile_arguments.push_back(include_string_argument.c_str());

	// Keep compile_arguments_wstring alive when passing compile_arguments_lpcwstr to compile
	std::vector<std::wstring> compile_arguments_wstring = std::to_wstring(compile_arguments);
	std::vector<LPCWSTR> compile_arguments_lpcwstr{};
	for (const std::wstring& argument : compile_arguments_wstring)
	{
		compile_arguments_lpcwstr.push_back(argument.c_str());
	}

	Microsoft::WRL::ComPtr<IDxcResult> compileResult{};
	m_compiler->Compile(&sourceBuffer, compile_arguments_lpcwstr.data(), (uint32)compile_arguments_lpcwstr.size(), m_include_handler.Get(), IID_PPV_ARGS(&compileResult)) >> CHK;

	Microsoft::WRL::ComPtr<IDxcBlobUtf8> pErrors{};
	compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr) >> CHK;
	if (pErrors && pErrors->GetStringLength() > 0)
	{
		LogError("{0}", (char*)pErrors->GetBufferPointer());
	}
	HRESULT HR{};
	compileResult->GetStatus(&HR) >> CHK;
	HR >> CHK;

	compileResult->GetOutput(DXC_OUT_OBJECT, __uuidof(IDxcBlob), (void**)&shader_blob, nullptr) >> CHK;

	return Shader{shader_blob, shader_desc};
}

