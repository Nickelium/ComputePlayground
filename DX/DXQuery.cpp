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

D3D_FEATURE_LEVEL GetMaxFeatureLevel(ComPtr<ID3D12Device> device)
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

D3D_FEATURE_LEVEL GetMaxFeatureLevel(ComPtr<IDXGIAdapter> adapter)
{
	// Require to support atleast bare d3d12
	ComPtr<ID3D12Device9> device{};
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

static const std::map< D3D_SHADER_MODEL, std::string> g_shader_model_map_string =
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

D3D_SHADER_MODEL GetMaxShaderModel(ComPtr<ID3D12Device> device)
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
static const std::string g_dxgi_format_map_string[] =
{
	"DXGI_FORMAT_UNKNOWN",
	"DXGI_FORMAT_R32G32B32A32_TYPELESS",
	"DXGI_FORMAT_R32G32B32A32_FLOAT",
	"DXGI_FORMAT_R32G32B32A32_UINT",
	"DXGI_FORMAT_R32G32B32A32_SINT",
	"DXGI_FORMAT_R32G32B32_TYPELESS",
	"DXGI_FORMAT_R32G32B32_FLOAT",
	"DXGI_FORMAT_R32G32B32_UINT",
	"DXGI_FORMAT_R32G32B32_SINT",
	"DXGI_FORMAT_R16G16B16A16_TYPELESS",
	"DXGI_FORMAT_R16G16B16A16_FLOAT",
	"DXGI_FORMAT_R16G16B16A16_UNORM",
	"DXGI_FORMAT_R16G16B16A16_UINT",
	"DXGI_FORMAT_R16G16B16A16_SNORM",
	"DXGI_FORMAT_R16G16B16A16_SINT",
	"DXGI_FORMAT_R32G32_TYPELESS",
	"DXGI_FORMAT_R32G32_FLOAT",
	"DXGI_FORMAT_R32G32_UINT",
	"DXGI_FORMAT_R32G32_SINT",
	"DXGI_FORMAT_R32G8X24_TYPELESS",
	"DXGI_FORMAT_D32_FLOAT_S8X24_UINT",
	"DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS",
	"DXGI_FORMAT_X32_TYPELESS_G8X24_UINT",
	"DXGI_FORMAT_R10G10B10A2_TYPELESS",
	"DXGI_FORMAT_R10G10B10A2_UNORM",
	"DXGI_FORMAT_R10G10B10A2_UINT",
	"DXGI_FORMAT_R11G11B10_FLOAT",
	"DXGI_FORMAT_R8G8B8A8_TYPELESS",
	"DXGI_FORMAT_R8G8B8A8_UNORM",
	"DXGI_FORMAT_R8G8B8A8_UNORM_SRGB",
	"DXGI_FORMAT_R8G8B8A8_UINT",
	"DXGI_FORMAT_R8G8B8A8_SNORM",
	"DXGI_FORMAT_R8G8B8A8_SINT",
	"DXGI_FORMAT_R16G16_TYPELESS",
	"DXGI_FORMAT_R16G16_FLOAT",
	"DXGI_FORMAT_R16G16_UNORM",
	"DXGI_FORMAT_R16G16_UINT",
	"DXGI_FORMAT_R16G16_SNORM",
	"DXGI_FORMAT_R16G16_SINT",
	"DXGI_FORMAT_R32_TYPELESS",
	"DXGI_FORMAT_D32_FLOAT",
	"DXGI_FORMAT_R32_FLOAT",
	"DXGI_FORMAT_R32_UINT",
	"DXGI_FORMAT_R32_SINT",
	"DXGI_FORMAT_R24G8_TYPELESS",
	"DXGI_FORMAT_D24_UNORM_S8_UINT",
	"DXGI_FORMAT_R24_UNORM_X8_TYPELESS",
	"DXGI_FORMAT_X24_TYPELESS_G8_UINT",
	"DXGI_FORMAT_R8G8_TYPELESS",
	"DXGI_FORMAT_R8G8_UNORM",
	"DXGI_FORMAT_R8G8_UINT",
	"DXGI_FORMAT_R8G8_SNORM",
	"DXGI_FORMAT_R8G8_SINT",
	"DXGI_FORMAT_R16_TYPELESS",
	"DXGI_FORMAT_R16_FLOAT",
	"DXGI_FORMAT_D16_UNORM",
	"DXGI_FORMAT_R16_UNORM",
	"DXGI_FORMAT_R16_UINT",
	"DXGI_FORMAT_R16_SNORM",
	"DXGI_FORMAT_R16_SINT",
	"DXGI_FORMAT_R8_TYPELESS",
	"DXGI_FORMAT_R8_UNORM",
	"DXGI_FORMAT_R8_UINT",
	"DXGI_FORMAT_R8_SNORM",
	"DXGI_FORMAT_R8_SINT",
	"DXGI_FORMAT_A8_UNORM",
	"DXGI_FORMAT_R1_UNORM",
	"DXGI_FORMAT_R9G9B9E5_SHAREDEXP",
	"DXGI_FORMAT_R8G8_B8G8_UNORM",
	"DXGI_FORMAT_G8R8_G8B8_UNORM",
	"DXGI_FORMAT_BC1_TYPELESS",
	"DXGI_FORMAT_BC1_UNORM",
	"DXGI_FORMAT_BC1_UNORM_SRGB",
	"DXGI_FORMAT_BC2_TYPELESS",
	"DXGI_FORMAT_BC2_UNORM",
	"DXGI_FORMAT_BC2_UNORM_SRGB",
	"DXGI_FORMAT_BC3_TYPELESS",
	"DXGI_FORMAT_BC3_UNORM",
	"DXGI_FORMAT_BC3_UNORM_SRGB",
	"DXGI_FORMAT_BC4_TYPELESS",
	"DXGI_FORMAT_BC4_UNORM",
	"DXGI_FORMAT_BC4_SNORM",
	"DXGI_FORMAT_BC5_TYPELESS",
	"DXGI_FORMAT_BC5_UNORM",
	"DXGI_FORMAT_BC5_SNORM",
	"DXGI_FORMAT_B5G6R5_UNORM",
	"DXGI_FORMAT_B5G5R5A1_UNORM",
	"DXGI_FORMAT_B8G8R8A8_UNORM",
	"DXGI_FORMAT_B8G8R8X8_UNORM",
	"DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM",
	"DXGI_FORMAT_B8G8R8A8_TYPELESS",
	"DXGI_FORMAT_B8G8R8A8_UNORM_SRGB",
	"DXGI_FORMAT_B8G8R8X8_TYPELESS",
	"DXGI_FORMAT_B8G8R8X8_UNORM_SRGB",
	"DXGI_FORMAT_BC6H_TYPELESS = 94",
	"DXGI_FORMAT_BC6H_UF16",
	"DXGI_FORMAT_BC6H_SF16",
	"DXGI_FORMAT_BC7_TYPELESS",
	"DXGI_FORMAT_BC7_UNORM",
	"DXGI_FORMAT_BC7_UNORM_SRGB",
	"DXGI_FORMAT_AYUV",
	"DXGI_FORMAT_Y410",
	"DXGI_FORMAT_Y416",
	"DXGI_FORMAT_NV12",
	"DXGI_FORMAT_P010",
	"DXGI_FORMAT_P016",
	"DXGI_FORMAT_420_OPAQUE",
	"DXGI_FORMAT_YUY2",
	"DXGI_FORMAT_Y210",
	"DXGI_FORMAT_Y216",
	"DXGI_FORMAT_NV11",
	"DXGI_FORMAT_AI44",
	"DXGI_FORMAT_IA44",
	"DXGI_FORMAT_P8",
	"DXGI_FORMAT_A8P8",
	"DXGI_FORMAT_B4G4R4A4_UNORM",
};

static_assert(COUNT(g_dxgi_format_map_string) == g_dxgi_format_last_stored + 1);

std::string GetDXGIFormatString(const DXGI_FORMAT& dxgi_format)
{
	ASSERT(dxgi_format <= g_dxgi_format_last_stored);
	ASSERT(dxgi_format < COUNT(g_dxgi_format_map_string));
	return g_dxgi_format_map_string[dxgi_format];
}

D3D12_RESOURCE_BINDING_TIER GetResourceBindingTier(ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options)) >> CHK;
	return options.ResourceBindingTier;
}

D3D12_RESOURCE_HEAP_TIER GetResourceHeapTier(ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options)) >> CHK;
	return options.ResourceHeapTier;
}

bool IsDXGIFormatSupported(ComPtr<ID3D12Device> device, const DXGI_FORMAT& format)
{
	D3D12_FEATURE_DATA_FORMAT_SUPPORT options
	{
		.Format = format
	};
	const HRESULT& result = device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &options, sizeof(options));
	return result == S_OK;
}

D3D_ROOT_SIGNATURE_VERSION GetMaxRootSignature(ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE  options 
	{ 
		.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1
	};
	device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &options, sizeof(options)) >> CHK;
	return options.HighestVersion;
}

D3D12_RAYTRACING_TIER GetRaytracingTier(ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options, sizeof(options)) >> CHK;
	return options.RaytracingTier;
}

uint64 GetWaveLaneCountMin(ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS1  options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &options, sizeof(options)) >> CHK;
	return options.WaveLaneCountMin;
}

uint64 GetWaveLaneCountMax(ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS1  options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &options, sizeof(options)) >> CHK;
	return options.WaveLaneCountMax;
}

D3D12_VARIABLE_SHADING_RATE_TIER GetVariableShadingRateTier(ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS6 options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options, sizeof(options)) >> CHK;
	return options.VariableShadingRateTier;
}

D3D12_MESH_SHADER_TIER GetMeshShaderTier(ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS7 options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options, sizeof(options)) >> CHK;
	return options.MeshShaderTier;
}

D3D12_SAMPLER_FEEDBACK_TIER GetSamplerFeedbackTier(ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS7 options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options, sizeof(options)) >> CHK;
	return options.SamplerFeedbackTier;
}

bool GetEnhancedBarrierSupported(ComPtr<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS12 options{};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options, sizeof(options)) >> CHK;
	return options.EnhancedBarriersSupported;
}

std::string DumpDX12Capabilities(ComPtr<ID3D12Device> device)
{
	std::vector<std::pair<std::string, std::string>> pair_data{};

	pair_data.push_back({ "ResourceBindingTier", std::format("{0}", (uint32)GetResourceBindingTier(device)) });
	pair_data.push_back({ "ResourceHeapTier", std::format("{0}", (uint32)GetResourceHeapTier(device)) });
	pair_data.push_back({ "MaxSupportedFeatureLevel", std::format("{0}", GetFeatureLevelString(GetMaxFeatureLevel(device))) });
	pair_data.push_back({ "HighestShaderModel", std::format("{0}", GetShaderModelString(GetMaxShaderModel(device))) });
	pair_data.push_back({ "RootSignatureHighestVersion", std::format("{0}", (uint32)GetMaxRootSignature(device)) });
	pair_data.push_back({ "RaytracingTier", std::format("{0}", (uint32)GetRaytracingTier(device)) });
	pair_data.push_back({ "VariableShadingRateTier", std::format("{0}", (uint32)GetVariableShadingRateTier(device)) });
	pair_data.push_back({ "MeshShaderTier", std::format("{0}", (uint32)GetMeshShaderTier(device)) });
	pair_data.push_back({ "SamplerFeedbackTier", std::format("{0}", (uint32)GetSamplerFeedbackTier(device)) });
	pair_data.push_back({ "EnhancedBarrier", std::format("{0}", GetEnhancedBarrierSupported(device)) });
	pair_data.push_back({ "WaveLaneCountMin", std::format("{0}", GetWaveLaneCountMin(device)) });
	pair_data.push_back({ "WaveLaneCountMax", std::format("{0}", GetWaveLaneCountMax(device)) });

	std::string ret{};
	for (auto& P : pair_data)
	{
		ret += std::format("{0}\t{1}\n", P.first, P.second);
	}
	return ret;
}