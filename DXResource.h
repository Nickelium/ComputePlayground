#pragma once

#include "Common.h"
#include "DXCommon.h"

struct UAV
{
	UAV();

	D3D12_RESOURCE_DESC m_desc = {};
	D3D12_HEAP_PROPERTIES m_heap_properties = {};
	ComPtr<ID3D12Resource> m_gpu_resource;

	D3D12_RESOURCE_DESC m_readback_desc = {};
	D3D12_HEAP_PROPERTIES m_readback_heap_properties = {};
	ComPtr<ID3D12Resource> m_read_back_resource;
};
