#pragma once

#include "../core/Common.h"
#include "DXCommon.h"

class DXResource
{
public:
	D3D12_RESOURCE_STATES m_resource_state = D3D12_RESOURCE_STATE_COMMON;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
private:
};

struct HeapResource
{
	HeapResource(D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_FLAGS resource_flags, uint32 size);

	D3D12_RESOURCE_DESC m_desc;
	D3D12_HEAP_PROPERTIES m_heap_properties;
	DXResource m_dx_resource;
};
