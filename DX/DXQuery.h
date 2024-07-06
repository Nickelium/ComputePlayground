#pragma once
#include "../core/Common.h"
#include "DXCommon.h"

D3D_FEATURE_LEVEL GetMaxFeatureLevel(Microsoft::WRL::ComPtr<ID3D12Device> device);
D3D_FEATURE_LEVEL GetMaxFeatureLevel(Microsoft::WRL::ComPtr<IDXGIAdapter> adapter);
std::string GetFeatureLevelString(const D3D_FEATURE_LEVEL& feature_level);

D3D_SHADER_MODEL GetMaxShaderModel(Microsoft::WRL::ComPtr<ID3D12Device> device);
std::string GetShaderModelString(const D3D_SHADER_MODEL& shader_model);

bool IsDXGIFormatSupported(Microsoft::WRL::ComPtr<ID3D12Device> device, const DXGI_FORMAT& format);
std::string GetDXGIFormatString(const DXGI_FORMAT& dxgi_format);

bool GetSupportDynamicResourceBinding(Microsoft::WRL::ComPtr<ID3D12Device> device);
D3D12_RESOURCE_BINDING_TIER GetResourceBindingTier(Microsoft::WRL::ComPtr<ID3D12Device> device);
D3D12_RESOURCE_HEAP_TIER GetResourceHeapTier(Microsoft::WRL::ComPtr<ID3D12Device> device);
D3D_ROOT_SIGNATURE_VERSION GetMaxRootSignature(Microsoft::WRL::ComPtr<ID3D12Device> device);
D3D12_RAYTRACING_TIER GetRaytracingTier(Microsoft::WRL::ComPtr<ID3D12Device> device);
D3D12_VARIABLE_SHADING_RATE_TIER GetVariableShadingRateTier(Microsoft::WRL::ComPtr<ID3D12Device> device);
D3D12_MESH_SHADER_TIER GetMeshShaderTier(Microsoft::WRL::ComPtr<ID3D12Device> device);
D3D12_SAMPLER_FEEDBACK_TIER GetSamplerFeedbackTier(Microsoft::WRL::ComPtr<ID3D12Device> device);
bool GetEnhancedBarrierSupported(Microsoft::WRL::ComPtr<ID3D12Device> device);
bool GetBindlessSupport(Microsoft::WRL::ComPtr<ID3D12Device> device);
uint64 GetVRAMUsage(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter);

std::string DumpDX12Capabilities(Microsoft::WRL::ComPtr<ID3D12Device> device);