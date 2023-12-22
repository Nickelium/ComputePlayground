#pragma once
#include "Common.h"
#include "IDXDebugLayer.h"

#if defined(_DEBUG)

struct IDXGIDebug1;
struct ID3D12Debug5;
void PIXCaptureAndOpen();
enum class GRAPHICS_DEBUGGER_TYPE
{
	PIX,
	RENDERDOC,
};
class DXDebugLayer : public IDXDebugLayer
{
public:
	DXDebugLayer(GRAPHICS_DEBUGGER_TYPE type);
	virtual ~DXDebugLayer();
	virtual void Init() override;
	virtual void Close() override;
private:
	ComPtr<IDXGIDebug1> m_dxgiDebug;
	ComPtr<ID3D12Debug5> m_d3d12Debug;
	HMODULE m_pixModule;
	HMODULE m_renderdocModule;

	GRAPHICS_DEBUGGER_TYPE m_graphicsDebuggerType;
};
#endif