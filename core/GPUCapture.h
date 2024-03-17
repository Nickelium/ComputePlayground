#pragma once
#include "Common.h"
#include "../DX/IDXDebugLayer.h"

struct IDXGIDebug1;
struct ID3D12Debug5;
struct RENDERDOC_API_1_6_0;

// TODO split out renderdoc and pix
class GPUCapture
{
public:
	GPUCapture(const GRAPHICS_DEBUGGER_TYPE gd_type);
	~GPUCapture();

	void PIXCaptureAndOpen();
	void RenderdocCaptureStart();
	void RenderdocCaptureEnd();
private:
	void Init();
	void Close();

	HMODULE m_pix_module;

	HMODULE m_renderdoc_module;
	RENDERDOC_API_1_6_0* m_renderdoc_api;

	GRAPHICS_DEBUGGER_TYPE m_graphics_debugger_type;
};