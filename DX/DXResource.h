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


// Use CBV for faster loads when using uniform indexing access pattern acrosss wave
// It uses on NVIDIA a hardware cache but CBV has 256 alignment requirements due to some older GPUs
// Also CBV has max size of 64kB
// SRV has less penalty with different access pattern
using SRV = DXDescriptor;
using UAV = DXDescriptor;
using CBV = DXDescriptor;

class DXResource
{
public:
	uint64 m_size_in_bytes;

	D3D12_RESOURCE_STATES m_resource_state = D3D12_RESOURCE_STATE_COMMON;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

	D3D12_RESOURCE_DESC m_resource_desc;
	// Some have heap, some not
	D3D12_HEAP_PROPERTIES m_heap_properties;

	virtual void SetResourceInfo(D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_FLAGS resource_flags, uint64 bytes);
	virtual void CreateResource(DXContext& dx_context, const std::string& name_resource);
	
	// TODO reserved resources
//	virtual void AllocateVirtual(DXContext& dx_context);
//	virtual void AllocatePhysical(DXContext& dx_context);
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