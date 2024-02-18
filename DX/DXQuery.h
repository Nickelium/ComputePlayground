#pragma once
#include "../Common.h"
#include "DXCommon.h"

D3D_FEATURE_LEVEL GetMaxFeatureLevel(ComPtr<ID3D12Device> device);
D3D_FEATURE_LEVEL GetMaxFeatureLevel(ComPtr<IDXGIAdapter> adapter);
std::string GetFeatureLevelString(const D3D_FEATURE_LEVEL& feature_level);

D3D_SHADER_MODEL GetMaxShaderModel(ComPtr<ID3D12Device> device);
std::string GetShaderModelString(const D3D_SHADER_MODEL& shader_model);

bool IsDXGIFormatSupported(ComPtr<ID3D12Device> device, const DXGI_FORMAT& format);
std::string GetDXGIFormatString(const DXGI_FORMAT& dxgi_format);

D3D12_RESOURCE_BINDING_TIER GetResourceBindingTier(ComPtr<ID3D12Device> device);
D3D12_RESOURCE_HEAP_TIER GetResourceHeapTier(ComPtr<ID3D12Device> device);
D3D_ROOT_SIGNATURE_VERSION GetMaxRootSignature(ComPtr<ID3D12Device> device);
D3D12_RAYTRACING_TIER GetRaytracingTier(ComPtr<ID3D12Device> device);
D3D12_VARIABLE_SHADING_RATE_TIER GetVariableShadingRateTier(ComPtr<ID3D12Device> device);
D3D12_MESH_SHADER_TIER GetMeshShaderTier(ComPtr<ID3D12Device> device);
D3D12_SAMPLER_FEEDBACK_TIER GetSamplerFeedbackTier(ComPtr<ID3D12Device> device);
bool GetEnhancedBarrierSupported(ComPtr<ID3D12Device> device);

std::string DumpDX12Capabilities(ComPtr<ID3D12Device> device);