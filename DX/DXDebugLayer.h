#pragma once
#include "../Common.h"
#include "IDXDebugLayer.h"

struct IDXGIDebug1;
struct ID3D12Debug5;
struct RENDERDOC_API_1_6_0;

class DXDebugLayer
{
public:
	DXDebugLayer(const GRAPHICS_DEBUGGER_TYPE gd_type);
	~DXDebugLayer();
	void Init();
	void Close();

	void PIXCaptureAndOpen();
	void RenderdocCaptureStart();
	void RenderdocCaptureEnd();
private:
	HMODULE m_pix_module;

	HMODULE m_renderdoc_module;
	RENDERDOC_API_1_6_0* m_renderdoc_api;

	GRAPHICS_DEBUGGER_TYPE m_graphics_debugger_type;
};