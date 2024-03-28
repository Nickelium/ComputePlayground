#pragma once

#include "../core/Common.h"
#include "DXCommon.h"

class DXResource
{
public:
	D3D12_RESOURCE_STATES m_resource_state;
	ComPtr<ID3D12Resource> m_resource;
private:
};

struct UAV
{
	UAV();

	// GPU resource
	D3D12_RESOURCE_DESC m_desc;
	D3D12_HEAP_PROPERTIES m_heap_properties;
	ComPtr<ID3D12Resource> m_gpu_resource;
	D3D12_RESOURCE_STATES m_gpu_resource_state;

	// Readback resource
	D3D12_RESOURCE_DESC m_readback_desc;
	D3D12_HEAP_PROPERTIES m_readback_heap_properties;
	ComPtr<ID3D12Resource> m_read_back_resource;
	D3D12_RESOURCE_STATES m_readback_resource_state;
};
