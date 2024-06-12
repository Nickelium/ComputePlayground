#pragma once

#include "../core/Common.h"
#include "DXCommon.h"

class DXContext;
class DXResource
{
public:
	D3D12_RESOURCE_STATES m_resource_state = D3D12_RESOURCE_STATE_COMMON;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

	// Some have heap, some not
	D3D12_RESOURCE_DESC m_resource_desc;
	D3D12_HEAP_PROPERTIES m_heap_properties;

	void SetResourceInfo(D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_FLAGS resource_flags, uint32 size);
	virtual void CreateResource(DXContext& dx_context, const std::string& name_resource);
};

class DXVertexBufferResource : public DXResource
{
public:
	uint32 m_stride;
	uint32 m_size;
	uint32 m_count;

	D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view{};

	void SetResourceInfo(D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_FLAGS resource_flags, uint32 size, uint32 stride);
	virtual void CreateResource(DXContext& dx_context, const std::string& name_resource) override;
};