#pragma once

#include "../core/Common.h"
#include "DXCommon.h"

class DXContext;

class DXDescriptor
{
public:
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_descriptor_handle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_descriptor_handle;

	uint32 m_bindless_index = ~0u;
};

class TypedBufferView
{
	D3D12_UAV_DIMENSION m_view_dimension = D3D12_UAV_DIMENSION_BUFFER;
	constexpr static uint32 s_stride = 0;
	DXGI_FORMAT m_format;
	D3D12_BUFFER_SRV_FLAGS m_flags_srv = D3D12_BUFFER_SRV_FLAG_NONE;
	D3D12_BUFFER_UAV_FLAGS m_flags_uav = D3D12_BUFFER_UAV_FLAG_NONE;
};
template<typename T>
class StructuredBufferView
{
	D3D12_UAV_DIMENSION m_view_dimension = D3D12_UAV_DIMENSION_BUFFER;
	// bytes
	constexpr static uint32 s_stride = sizeof(T);
	DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
	D3D12_BUFFER_SRV_FLAGS m_flags_srv = D3D12_BUFFER_SRV_FLAG_NONE;
	D3D12_BUFFER_UAV_FLAGS m_flags_uav = D3D12_BUFFER_UAV_FLAG_NONE;
};
class ByteBufferView
{
	D3D12_UAV_DIMENSION m_view_dimension = D3D12_UAV_DIMENSION_BUFFER;
	constexpr static uint32 s_stride = 0;
	DXGI_FORMAT m_format = DXGI_FORMAT_R32_TYPELESS;
	D3D12_BUFFER_SRV_FLAGS m_flags_srv = D3D12_BUFFER_SRV_FLAG_RAW;
	D3D12_BUFFER_UAV_FLAGS m_flags_uav = D3D12_BUFFER_UAV_FLAG_RAW;
};

using SRV = DXDescriptor;
using UAV = DXDescriptor;
using CBV = DXDescriptor;

class DXResource
{
public:
	uint32 m_size;

	D3D12_RESOURCE_STATES m_resource_state = D3D12_RESOURCE_STATE_COMMON;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

	D3D12_RESOURCE_DESC m_resource_desc;
	// Some have heap, some not
	D3D12_HEAP_PROPERTIES m_heap_properties;

	virtual void SetResourceInfo(D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_FLAGS resource_flags, uint32 size);
	virtual void CreateResource(DXContext& dx_context, const std::string& name_resource);
};

class DXVertexBufferResource : public DXResource
{
public:
	uint32 m_stride;
	uint32 m_count;

	D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view{};

	void SetResourceInfo(D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_FLAGS resource_flags, uint32 size, uint32 stride);
	virtual void CreateResource(DXContext& dx_context, const std::string& name_resource) override;
};

class DXTextureResource : public DXResource
{
public:
	uint32 m_width;
	uint32 m_height;
	DXGI_FORMAT m_format;

	virtual void SetResourceInfo(D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_FLAGS resource_flags, uint32 width, uint32 height, DXGI_FORMAT format);
	virtual void CreateResource(DXContext& dx_context, const std::string& name_resource);
};


extern uint32 g_current_buffer_index;
// Ref count, deferred deletion till GPU is done using
class ResourceHandler
{
public:
	// Create Transient Resource
	void RegisterResource(DXResource& resource)
	{
		m_resources[g_current_buffer_index].push_back(resource);
	}

	void FreeResources()
	{
		m_resources[g_current_buffer_index].clear();
	}
	// Create Persistent Resource
	std::vector<DXResource> m_resources[g_backbuffer_count];
};