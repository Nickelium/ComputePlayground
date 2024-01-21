#pragma once
#include "../Common.h"
#include "IDXDebugLayer.h"

struct IDXGIDebug1;
struct ID3D12Debug5;
struct RENDERDOC_API_1_6_0;
void PIXCaptureAndOpen();
class DXDebugLayer : public IDXDebugLayer
{
public:
	DXDebugLayer(GRAPHICS_DEBUGGER_TYPE gd_type);
	virtual ~DXDebugLayer();
	virtual void Init() override;
	void Close();

	virtual void PIXCaptureAndOpen() override;
	virtual void RenderdocCaptureStart() override;
	virtual void RenderdocCaptureEnd() override;
private:
	ComPtr<IDXGIDebug1> m_dxgi_debug;
	ComPtr<ID3D12Debug5> m_d3d12_debug;
	HMODULE m_pix_module;

	HMODULE m_renderdoc_module;
	RENDERDOC_API_1_6_0* m_renderdoc_api;

	GRAPHICS_DEBUGGER_TYPE m_graphics_debugger_type;
};