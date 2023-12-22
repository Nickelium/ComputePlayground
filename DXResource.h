#pragma once

#include "Common.h"
#include "DXCommon.h"

struct UAV
{
	UAV();

	D3D12_RESOURCE_DESC m_Desc = {};
	D3D12_HEAP_PROPERTIES m_HeapProperties = {};
	ComPtr<ID3D12Resource> m_GPUResource;

	D3D12_RESOURCE_DESC m_ReadbackDesc = {};
	D3D12_HEAP_PROPERTIES m_ReadbackHeapProperties = {};
	ComPtr<ID3D12Resource> m_ReadbackResource;
};
