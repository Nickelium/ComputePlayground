#include "DXQuery.h"

#include <array>
#include <map>
#include <format>

// All D3D12 compliant feature levels
static const D3D_FEATURE_LEVEL g_feature_levels[] =
{
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_11_1,
	D3D_FEATURE_LEVEL_12_0,
	D3D_FEATURE_LEVEL_12_1,
	D3D_FEATURE_LEVEL_12_2
};

static const std::map<D3D_FEATURE_LEVEL, std::string> g_feature_levels_map_string =
{
	{D3D_FEATURE_LEVEL_11_0, "11_0"},
	{D3D_FEATURE_LEVEL_11_1, "11_1"},
	{D3D_FEATURE_LEVEL_12_0, "12_0"},
	{D3D_FEATURE_LEVEL_12_1, "12_1"},
	{D3D_FEATURE_LEVEL_12_2, "12_2"},
};

static const D3D_FEATURE_LEVEL min_feature_level = g_feature_levels[0];

D3D_FEATURE_LEVEL GetMaxFeatureLevel(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	const D3D12_FEATURE feature{ D3D12_FEATURE_FEATURE_LEVELS };
	D3D12_FEATURE_DATA_FEATURE_LEVELS feature_data =
	{
		.NumFeatureLevels = COUNT(g_feature_levels),
		.pFeatureLevelsRequested = g_feature_levels,
	};
	device->CheckFeatureSupport(feature, &feature_data, sizeof(feature_data)) >> CHK;
	return feature_data.MaxSupportedFeatureLevel;
}

D3D_FEATURE_LEVEL GetMaxFeatureLevel(Microsoft::WRL::ComPtr<IDXGIAdapter> adapter)
{
	// Require to support atleast bare d3d12
	Microsoft::WRL::ComPtr<ID3D12Device9> device{};
	D3D12CreateDevice(adapter.Get(), min_feature_level, IID_PPV_ARGS(&device)) >> CHK;
	return GetMaxFeatureLevel(device);
}

std::string GetFeatureLevelString(const D3D_FEATURE_LEVEL& feature_level)
{
	const auto iterator = g_feature_levels_map_string.find(feature_level);
	if (iterator != g_feature_levels_map_string.cend())
	{
		return iterator->second;
	}
	ASSERT(false && "Invalid feature level");
	return "";
}

// Shader models compatible with DX12
static const D3D_SHADER_MODEL g_shader_models[] =
{
	D3D_SHADER_MODEL_5_1,
	D3D_SHADER_MODEL_6_0,
	D3D_SHADER_MODEL_6_1,
	D3D_SHADER_MODEL_6_2,
	D3D_SHADER_MODEL_6_3,
	D3D_SHADER_MODEL_6_4,
	D3D_SHADER_MODEL_6_5,
	D3D_SHADER_MODEL_6_6,
	D3D_SHADER_MODEL_6_7,
	D3D_SHADER_MODEL_6_8,
};

static const D3D_SHADER_MODEL g_lowest_shader_model = g_shader_models[0];

static const std::map<D3D_SHADER_MODEL, std::string> g_shader_model_map_string =
{
	{D3D_SHADER_MODEL_5_1, "5_1"},
	{D3D_SHADER_MODEL_6_0, "6_0"},
	{D3D_SHADER_MODEL_6_1, "6_1"},
	{D3D_SHADER_MODEL_6_2, "6_2"},
	{D3D_SHADER_MODEL_6_3, "6_3"},
	{D3D_SHADER_MODEL_6_4, "6_4"},
	{D3D_SHADER_MODEL_6_5, "6_5"},
	{D3D_SHADER_MODEL_6_6, "6_6"},
	{D3D_SHADER_MODEL_6_7, "6_7"},
	{D3D_SHADER_MODEL_6_8, "6_8"},
};

D3D_SHADER_MODEL GetMaxShaderModel(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	for (int32 i = (COUNT(g_shader_models) - 1); i >= 0; --i)
	{
		D3D12_FEATURE_DATA_SHADER_MODEL shader_model_option{};
		shader_model_option.HighestShaderModel = { g_shader_models[i] };
		const HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shader_model_option, sizeof(shader_model_option));
		if (hr == S_OK)
		{
			return shader_model_option.HighestShaderModel;
		}
	}
	ASSERT(false && "None of the shader models are supported");
	return g_lowest_shader_model;
}

std::string GetShaderModelString(const D3D_SHADER_MODEL& shader_model)
{
	const auto& iterator = g_shader_model_map_string.find(shader_model);
	if (iterator != g_shader_model_map_string.cend())
	{
		return iterator->second;
	}
	ASSERT(false && "Shader model invalid or not supported");
	return "";
}

// No easier way to do since DXGI format is declarared in internal dxgiformat.h header already
// static const DXGI_FORMAT dxgi_last_element = DXGI_FORMAT_A4B4G4R4_UNORM;
// Keep in sync with DXGI_FORMAT
// Only stored continuous values till DXGI_FORMAT_B4G4R4A4_UNORM = 115
static const DXGI_FORMAT g_dxgi_format_last_stored = DXGI_FORMAT_B4G4R4A4_UNORM;

static const std::map<DXGI_FORMAT, std::string> g_dxgi_format_map_string =
{
	{ DXGI_FORMAT_UNKNOWN	                             , "UNKNOWN"},
	{DXGI_FORMAT_R32G32B32A32_TYPELESS                   ,"R32G32B32A32_TYPELESS"},
	{DXGI_FORMAT_R32G32B32A32_FLOAT                      ,"R32G32B32A32_FLOAT"},
	{DXGI_FORMAT_R32G32B32A32_UINT                       ,"R32G32B32A32_UINT"},
	{DXGI_FORMAT_R32G32B32A32_SINT                       ,"R32G32B32A32_SINT"},
	{DXGI_FORMAT_R32G32B32_TYPELESS                      ,"R32G32B32_TYPELESS"},
	{DXGI_FORMAT_R32G32B32_FLOAT                         ,"R32G32B32_FLOAT"},
	{DXGI_FORMAT_R32G32B32_UINT                          ,"R32G32B32_UINT"},
	{DXGI_FORMAT_R32G32B32_SINT                          ,"R32G32B32_SINT"},
	{DXGI_FORMAT_R16G16B16A16_TYPELESS                   ,"R16G16B16A16_TYPELESS"},
	{DXGI_FORMAT_R16G16B16A16_FLOAT                      ,"R16G16B16A16_FLOAT"},
	{DXGI_FORMAT_R16G16B16A16_UNORM                      ,"R16G16B16A16_UNORM"},
	{DXGI_FORMAT_R16G16B16A16_UINT                       ,"R16G16B16A16_UINT"},
	{DXGI_FORMAT_R16G16B16A16_SNORM                      ,"R16G16B16A16_SNORM"},
	{DXGI_FORMAT_R16G16B16A16_SINT                       ,"R16G16B16A16_SINT"},
	{DXGI_FORMAT_R32G32_TYPELESS                         ,"R32G32_TYPELESS"},
	{DXGI_FORMAT_R32G32_FLOAT                            ,"R32G32_FLOAT"},
	{DXGI_FORMAT_R32G32_UINT                             ,"R32G32_UINT"},
	{DXGI_FORMAT_R32G32_SINT                             ,"R32G32_SINT"},
	{DXGI_FORMAT_R32G8X24_TYPELESS                       ,"R32G8X24_TYPELESS"},
	{DXGI_FORMAT_D32_FLOAT_S8X24_UINT                    ,"D32_FLOAT_S8X24_UINT"},
	{DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS                ,"R32_FLOAT_X8X24_TYPELESS"},
	{DXGI_FORMAT_X32_TYPELESS_G8X24_UINT                 ,"X32_TYPELESS_G8X24_UINT"},
	{DXGI_FORMAT_R10G10B10A2_TYPELESS                    ,"R10G10B10A2_TYPELESS"},
	{DXGI_FORMAT_R10G10B10A2_UNORM                       ,"R10G10B10A2_UNORM"},
	{DXGI_FORMAT_R10G10B10A2_UINT                        ,"R10G10B10A2_UINT"},
	{DXGI_FORMAT_R11G11B10_FLOAT                         ,"R11G11B10_FLOAT"},
	{DXGI_FORMAT_R8G8B8A8_TYPELESS                       ,"R8G8B8A8_TYPELESS"},
	{DXGI_FORMAT_R8G8B8A8_UNORM                          ,"R8G8B8A8_UNORM"},
	{DXGI_FORMAT_R8G8B8A8_UNORM_SRGB                     ,"R8G8B8A8_UNORM_SRGB"},
	{DXGI_FORMAT_R8G8B8A8_UINT                           ,"R8G8B8A8_UINT"},
	{DXGI_FORMAT_R8G8B8A8_SNORM                          ,"R8G8B8A8_SNORM"},
	{DXGI_FORMAT_R8G8B8A8_SINT                           ,"R8G8B8A8_SINT"},
	{DXGI_FORMAT_R16G16_TYPELESS                         ,"R16G16_TYPELESS"},
	{DXGI_FORMAT_R16G16_FLOAT                            ,"R16G16_FLOAT"},
	{DXGI_FORMAT_R16G16_UNORM                            ,"R16G16_UNORM"},
	{DXGI_FORMAT_R16G16_UINT                             ,"R16G16_UINT"},
	{DXGI_FORMAT_R16G16_SNORM                            ,"R16G16_SNORM"},
	{DXGI_FORMAT_R16G16_SINT                             ,"R16G16_SINT"},
	{DXGI_FORMAT_R32_TYPELESS                            ,"R32_TYPELESS"},
	{DXGI_FORMAT_D32_FLOAT                               ,"D32_FLOAT"},
	{DXGI_FORMAT_R32_FLOAT                               ,"R32_FLOAT"},
	{DXGI_FORMAT_R32_UINT                                ,"R32_UINT"},
	{DXGI_FORMAT_R32_SINT                                ,"R32_SINT"},
	{DXGI_FORMAT_R24G8_TYPELESS                          ,"R24G8_TYPELESS"},
	{DXGI_FORMAT_D24_UNORM_S8_UINT                       ,"D24_UNORM_S8_UINT"},
	{DXGI_FORMAT_R24_UNORM_X8_TYPELESS                   ,"R24_UNORM_X8_TYPELESS"},
	{DXGI_FORMAT_X24_TYPELESS_G8_UINT                    ,"X24_TYPELESS_G8_UINT"},
	{DXGI_FORMAT_R8G8_TYPELESS                           ,"R8G8_TYPELESS"},
	{DXGI_FORMAT_R8G8_UNORM                              ,"R8G8_UNORM"},
	{DXGI_FORMAT_R8G8_UINT                               ,"R8G8_UINT"},
	{DXGI_FORMAT_R8G8_SNORM                              ,"R8G8_SNORM"},
	{DXGI_FORMAT_R8G8_SINT                               ,"R8G8_SINT"},
	{DXGI_FORMAT_R16_TYPELESS                            ,"R16_TYPELESS"},
	{DXGI_FORMAT_R16_FLOAT                               ,"R16_FLOAT"},
	{DXGI_FORMAT_D16_UNORM                               ,"D16_UNORM"},
	{DXGI_FORMAT_R16_UNORM                               ,"R16_UNORM"},
	{DXGI_FORMAT_R16_UINT                                ,"R16_UINT"},
	{DXGI_FORMAT_R16_SNORM                               ,"R16_SNORM"},
	{DXGI_FORMAT_R16_SINT                                ,"R16_SINT"},
	{DXGI_FORMAT_R8_TYPELESS                             ,"R8_TYPELESS"},
	{DXGI_FORMAT_R8_UNORM                                ,"R8_UNORM"},
	{DXGI_FORMAT_R8_UINT                                 ,"R8_UINT"},
	{DXGI_FORMAT_R8_SNORM                                ,"R8_SNORM"},
	{DXGI_FORMAT_R8_SINT                                 ,"R8_SINT"},
	{DXGI_FORMAT_A8_UNORM                                ,"A8_UNORM"},
	{DXGI_FORMAT_R1_UNORM                                ,"R1_UNORM"},
	{DXGI_FORMAT_R9G9B9E5_SHAREDEXP                      ,"R9G9B9E5_SHAREDEXP"},
	{DXGI_FORMAT_R8G8_B8G8_UNORM                         ,"R8G8_B8G8_UNORM"},
	{DXGI_FORMAT_G8R8_G8B8_UNORM                         ,"G8R8_G8B8_UNORM"},
	{DXGI_FORMAT_BC1_TYPELESS                            ,"BC1_TYPELESS"},
	{DXGI_FORMAT_BC1_UNORM                               ,"BC1_UNORM"},
	{DXGI_FORMAT_BC1_UNORM_SRGB                          ,"BC1_UNORM_SRGB"},
	{DXGI_FORMAT_BC2_TYPELESS                            ,"BC2_TYPELESS"},
	{DXGI_FORMAT_BC2_UNORM                               ,"BC2_UNORM"},
	{DXGI_FORMAT_BC2_UNORM_SRGB                          ,"BC2_UNORM_SRGB"},
	{DXGI_FORMAT_BC3_TYPELESS                            ,"BC3_TYPELESS"},
	{DXGI_FORMAT_BC3_UNORM                               ,"BC3_UNORM"},
	{DXGI_FORMAT_BC3_UNORM_SRGB                          ,"BC3_UNORM_SRGB"},
	{DXGI_FORMAT_BC4_TYPELESS                            ,"BC4_TYPELESS"},
	{DXGI_FORMAT_BC4_UNORM                               ,"BC4_UNORM"},
	{DXGI_FORMAT_BC4_SNORM                               ,"BC4_SNORM"},
	{DXGI_FORMAT_BC5_TYPELESS                            ,"BC5_TYPELESS"},
	{DXGI_FORMAT_BC5_UNORM                               ,"BC5_UNORM"},
	{DXGI_FORMAT_BC5_SNORM                               ,"BC5_SNORM"},
	{DXGI_FORMAT_B5G6R5_UNORM                            ,"B5G6R5_UNORM"},
	{DXGI_FORMAT_B5G5R5A1_UNORM                          ,"B5G5R5A1_UNORM"},
	{DXGI_FORMAT_B8G8R8A8_UNORM                          ,"B8G8R8A8_UNORM"},
	{DXGI_FORMAT_B8G8R8X8_UNORM                          ,"B8G8R8X8_UNORM"},
	{DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM              ,"R10G10B10_XR_BIAS_A2_UNORM"},
	{DXGI_FORMAT_B8G8R8A8_TYPELESS                       ,"B8G8R8A8_TYPELESS"},
	{DXGI_FORMAT_B8G8R8A8_UNORM_SRGB                     ,"B8G8R8A8_UNORM_SRGB"},
	{DXGI_FORMAT_B8G8R8X8_TYPELESS                       ,"B8G8R8X8_TYPELESS"},
	{DXGI_FORMAT_B8G8R8X8_UNORM_SRGB                     ,"B8G8R8X8_UNORM_SRGB"},
	{DXGI_FORMAT_BC6H_TYPELESS                           ,"BC6H_TYPELESS"},
	{DXGI_FORMAT_BC6H_UF16                               ,"BC6H_UF16"},
	{DXGI_FORMAT_BC6H_SF16                               ,"BC6H_SF16"},
	{DXGI_FORMAT_BC7_TYPELESS                            ,"BC7_TYPELESS"},
	{DXGI_FORMAT_BC7_UNORM                               ,"BC7_UNORM"},
	{DXGI_FORMAT_BC7_UNORM_SRGB                          ,"BC7_UNORM_SRGB"},
	{DXGI_FORMAT_AYUV                                    ,"AYUV"},
	{DXGI_FORMAT_Y410                                    ,"Y410"},
	{DXGI_FORMAT_Y416                                    ,"Y416"},
	{DXGI_FORMAT_NV12                                    ,"NV12"},
	{DXGI_FORMAT_P010                                    ,"P010"},
	{DXGI_FORMAT_P016                                    ,"P016"},
	{DXGI_FORMAT_420_OPAQUE                              ,"420_OPAQUE"},
	{DXGI_FORMAT_YUY2                                    ,"YUY2"},
	{DXGI_FORMAT_Y210                                    ,"Y210"},
	{DXGI_FORMAT_Y216                                    ,"Y216"},
	{DXGI_FORMAT_NV11                                    ,"NV11"},
	{DXGI_FORMAT_AI44                                    ,"AI44"},
	{DXGI_FORMAT_IA44                                    ,"IA44"},
	{DXGI_FORMAT_P8                                      ,"P8"},
	{DXGI_FORMAT_A8P8                                    ,"A8P8"},
	{DXGI_FORMAT_B4G4R4A4_UNORM                          ,"B4G4R4A4_UNORM"},
	{DXGI_FORMAT_P208                                    ,"P208"},
	{DXGI_FORMAT_V208                                    ,"P208"},
	{DXGI_FORMAT_V408                                    ,"V408"},
	{DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE         ,"SAMPLER_FEEDBACK_MIN_MIP_OPAQUE"},
	{DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE ,"SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE"},
	{DXGI_FORMAT_A4B4G4R4_UNORM                          ,"A4B4G4R4_UNORM"},
	{DXGI_FORMAT_FORCE_UINT                  			,"FORCE_UINT"},
};

std::string GetDXGIFormatString(const DXGI_FORMAT& dxgi_format)
{
	auto iterator = g_dxgi_format_map_string.find(dxgi_format);
	if (iterator != g_dxgi_format_map_string.cend())
	{
		return iterator->second;
	}
	ASSERT(false && "Invalid dxgi format");
	return "";
}

uint32 GetBytesFormat(DXGI_FORMAT format)
{
	switch(format)
	{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return 4 * 4;
		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 3 * 4;
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT :
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
			return 4 * 2;
		case DXGI_FORMAT_R32G32_TYPELESS :
		case DXGI_FORMAT_R32G32_FLOAT :
		case DXGI_FORMAT_R32G32_UINT :
		case DXGI_FORMAT_R32G32_SINT :
		case DXGI_FORMAT_R32G8X24_TYPELESS :
			return 2 * 4;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT :
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS :
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT :
		case DXGI_FORMAT_R10G10B10A2_TYPELESS :
		case DXGI_FORMAT_R10G10B10A2_UNORM :
		case DXGI_FORMAT_R10G10B10A2_UINT :
		case DXGI_FORMAT_R11G11B10_FLOAT :
		case DXGI_FORMAT_R8G8B8A8_TYPELESS :
		case DXGI_FORMAT_R8G8B8A8_UNORM :
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB :
		case DXGI_FORMAT_R8G8B8A8_UINT :
		case DXGI_FORMAT_R8G8B8A8_SNORM :
		case DXGI_FORMAT_R8G8B8A8_SINT :
		case DXGI_FORMAT_R16G16_TYPELESS :
		case DXGI_FORMAT_R16G16_FLOAT :
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT :
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT :
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT :
		case DXGI_FORMAT_R32_FLOAT :
		case DXGI_FORMAT_R32_UINT :
		case DXGI_FORMAT_R32_SINT :
		case DXGI_FORMAT_R24G8_TYPELESS :
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT :
			return 4;
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT :
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT :
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT :
		case DXGI_FORMAT_D16_UNORM :
		case DXGI_FORMAT_R16_UNORM :
		case DXGI_FORMAT_R16_UINT :
		case DXGI_FORMAT_R16_SNORM :
		case DXGI_FORMAT_R16_SINT :
			return 2;
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT :
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT :
		case DXGI_FORMAT_A8_UNORM:
			return 1;
		case DXGI_FORMAT_R1_UNORM:
			return 1; // This is actually 1 bit
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP :
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
			return 4;
		case DXGI_FORMAT_B5G6R5_UNORM :
		case DXGI_FORMAT_B5G5R5A1_UNORM :
			return 2;
		case DXGI_FORMAT_B8G8R8A8_UNORM :
		case DXGI_FORMAT_B8G8R8X8_UNORM :
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM :
		case DXGI_FORMAT_B8G8R8A8_TYPELESS :
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB :
		case DXGI_FORMAT_B8G8R8X8_TYPELESS :
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB :
			return 4;
		default:
			ASSERT(false && "Unsupported format");
			return 0;
	}
}

bool GetSupportDynamicResourceBinding(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	return 
		GetResourceBindingTier(device) >= D3D12_RESOURCE_BINDING_TIER_3 && 
		GetMaxShaderModel(device) >= D3D_SHADER_MODEL_6_6;
}

D3D12_RESOURCE_BINDING_TIER GetResourceBindingTier(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options)) >> CHK;
	return options.ResourceBindingTier;
}

D3D12_RESOURCE_HEAP_TIER GetResourceHeapTier(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options)) >> CHK;
	return options.ResourceHeapTier;
}

bool IsDXGIFormatSupported(Microsoft::WRL::ComPtr<ID3D12Device> device, const DXGI_FORMAT& format)
{
	D3D12_FEATURE_DATA_FORMAT_SUPPORT options
	{
		.Format = format
	};
	const HRESULT& result = device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &options, sizeof(options));
	return result == S_OK;
}

D3D_ROOT_SIGNATURE_VERSION GetMaxRootSignature(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE  options 
	{ 
		.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1
	};
	device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &options, sizeof(options)) >> CHK;
	return options.HighestVersion;
}

D3D12_RAYTRACING_TIER GetRaytracingTier(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options, sizeof(options)) >> CHK;
	return options.RaytracingTier;
}

uint64 GetWaveLaneCount(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS1 options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &options, sizeof(options)) >> CHK;
	return options.WaveLaneCountMin;
}

bool GetWorkGraphSupport(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS21 options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS21, &options, sizeof(options)) >> CHK;
	return options.WorkGraphsTier != D3D12_WORK_GRAPHS_TIER_NOT_SUPPORTED;
}

D3D12_VARIABLE_SHADING_RATE_TIER GetVariableShadingRateTier(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS6 options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options, sizeof(options)) >> CHK;
	return options.VariableShadingRateTier;
}

D3D12_MESH_SHADER_TIER GetMeshShaderTier(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS7 options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options, sizeof(options)) >> CHK;
	return options.MeshShaderTier;
}

D3D12_SAMPLER_FEEDBACK_TIER GetSamplerFeedbackTier(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS7 options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options, sizeof(options)) >> CHK;
	return options.SamplerFeedbackTier;
}

bool GetEnhancedBarrierSupport(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS12 options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options, sizeof(options)) >> CHK;
	return options.EnhancedBarriersSupported;
}

bool GetBindlessSupport(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	return GetResourceBindingTier(device) >= D3D12_RESOURCE_BINDING_TIER_3 && GetMaxShaderModel(device) >= D3D_SHADER_MODEL_6_6;
}

bool GetGPUUploadSupport(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS16 options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options, sizeof(options)) >> CHK;
	return options.GPUUploadHeapSupported;
}

//uint64 bytes_used, uint64 bytes_budget
std::pair<uint64, uint64> GetVRAM(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter)
{
	// Note that the budget can change by OS, eg. other instances and applications simultaneous running
	// video_memory_info.Budget > video_memory_info.CurrentUsage 
	// Respect budget such that memory is not demoted to system memory by the OS
	// node_index 0 because single GPU
	const uint32 node_index{0};
	// Local means non-system main memory, non CPU RAM, aka VRAM
	const DXGI_MEMORY_SEGMENT_GROUP memory_segment_group{ DXGI_MEMORY_SEGMENT_GROUP_LOCAL};
	DXGI_QUERY_VIDEO_MEMORY_INFO video_memory_info{};
	adapter->QueryVideoMemoryInfo(node_index, memory_segment_group, &video_memory_info) >> CHK;
	return { video_memory_info.CurrentUsage, video_memory_info.Budget };
}

std::string DumpDX12Capabilities(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	std::vector<std::pair<std::string, std::string>> pair_data{};

	pair_data.push_back({ "ResourceBindingTier", std::format("{0}", (uint32)GetResourceBindingTier(device)) });
	pair_data.push_back({ "SupportDynamicResourceBinding", std::format("{0}", GetSupportDynamicResourceBinding(device))} );
	pair_data.push_back({ "ResourceHeapTier", std::format("{0}", (uint32)GetResourceHeapTier(device)) });
	pair_data.push_back({ "MaxSupportedFeatureLevel", std::format("{0}", GetFeatureLevelString(GetMaxFeatureLevel(device))) });
	pair_data.push_back({ "HighestShaderModel", std::format("{0}", GetShaderModelString(GetMaxShaderModel(device))) });
	pair_data.push_back({ "RootSignatureHighestVersion", std::format("{0}", (uint32)GetMaxRootSignature(device)) });
	pair_data.push_back({ "RaytracingTier", std::format("{0}", (uint32)GetRaytracingTier(device)) });
	pair_data.push_back({ "VariableShadingRateTier", std::format("{0}", (uint32)GetVariableShadingRateTier(device)) });
	pair_data.push_back({ "MeshShaderTier", std::format("{0}", (uint32)GetMeshShaderTier(device)) });
	pair_data.push_back({ "SamplerFeedbackTier", std::format("{0}", (uint32)GetSamplerFeedbackTier(device)) });
	pair_data.push_back({ "EnhancedBarrier", std::format("{0}", GetEnhancedBarrierSupport(device)) });
	pair_data.push_back({ "WaveLaneCount", std::format("{0}", GetWaveLaneCount(device)) });
	pair_data.push_back({ "WorkGraph", std::format("{0}", GetWorkGraphSupport(device)) });
	pair_data.push_back({ "Bindless", std::format("{0}", GetBindlessSupport(device)) });
	pair_data.push_back({ "GPU Upload", std::format("{0}", GetGPUUploadSupport(device)) });

	std::string ret{};
	for (auto& P : pair_data)
	{
		ret += std::format("{0}\t{1}\n", P.first, P.second);
	}
	return ret;
}