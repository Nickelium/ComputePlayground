#pragma once
#include "../Common.h"
#include "IDXDebugLayer.h"

struct IDXGIDebug1;
struct ID3D12Debug5;
void PIXCaptureAndOpen();
class DXDebugLayer : public IDXDebugLayer
{
public:
	DXDebugLayer(GRAPHICS_DEBUGGER_TYPE gd_type);
	virtual ~DXDebugLayer();
	virtual void Init() override;
	void Close();
private:
	ComPtr<IDXGIDebug1> m_dxgi_debug;
	ComPtr<ID3D12Debug5> m_d3d12_debug;
	HMODULE m_pix_module;
	HMODULE m_renderdoc_module;

	GRAPHICS_DEBUGGER_TYPE m_graphics_debugger_type;
};