#pragma once
#include "DXCommon.h"
#include "DXContext.h"

struct PSO
{
	Microsoft::WRL::ComPtr<ID3D12StateObject> m_so;
	D3D12_PROGRAM_IDENTIFIER m_program_id;
};

PSO CreatePSO(DXContext& dx_context, CD3DX12_STATE_OBJECT_DESC& so_desc, const std::string& program_name);

