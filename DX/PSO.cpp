#include "PSO.h"

PSO CreatePSO(DXContext& dx_context, CD3DX12_STATE_OBJECT_DESC& so_desc, const std::string& program_name)
{
	PSO pso{};
	dx_context.GetDevice()->CreateStateObject(so_desc, IID_PPV_ARGS(&pso.m_so)) >> CHK;
	NAME_DX_OBJECT(pso.m_so, "State Object");

	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties1> state_object_properties;
	pso.m_so.As(&state_object_properties) >> CHK;
	const std::wstring& program_wname = std::to_wstring(program_name);
	pso.m_program_id = state_object_properties->GetProgramIdentifier(program_wname.c_str());
	return pso;
}