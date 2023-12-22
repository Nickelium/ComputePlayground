#include "DXDebugLayer.h"
#include "Common.h"
#include "DXCommon.h"

#if defined(_DEBUG)
#define USE_PIX
#include <pix3.h>
#endif

#include "C:\Program Files\RenderDoc\renderdoc_app.h"
// TODO crash on capture in NV driver?
HMODULE LoadRenderdoc()
{
	RENDERDOC_API_1_1_2* rdoc_api = nullptr;
	HMODULE renderdocModule = 0;
	bool success = GetModuleHandleExW(0, L"renderdoc.dll", &renderdocModule);
	if (!success)
	{
		renderdocModule = LoadLibraryExW(L"C://Program Files//RenderDoc//renderdoc.dll", NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	}
	pRENDERDOC_GetAPI RENDERDOC_GetAPI =
		(pRENDERDOC_GetAPI)GetProcAddress(renderdocModule, "RENDERDOC_GetAPI");
	int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&rdoc_api);
	assert(ret == 1);
	return renderdocModule;
}


#include <filesystem>
void PIXCaptureAndOpen()
{
#if defined(_DEBUG)
	std::wstring fileName = L".\\GPUPIXCapture";
	std::wstring extension = L".wpix";
	std::wstring identifier = L"";
	std::wstring fullName = fileName + identifier + extension;
	uint32_t index = 0;
	while (std::filesystem::exists(fullName))
	{
		identifier = L"_" + std::to_wstring(index);
		fullName = fileName + identifier + extension;
		++index;
	}
	PIXGpuCaptureNextFrames(fullName.c_str(), 1) >> CHK;
	ShellExecute(0, 0, fullName.c_str(), 0, 0, SW_SHOW);
#endif
}


DXDebugLayer::DXDebugLayer(GRAPHICS_DEBUGGER_TYPE type) :
	m_pixModule(0u),
	m_renderdocModule(0u),
	m_graphicsDebuggerType(type)
{

}

DXDebugLayer::~DXDebugLayer()
{

}

void DXDebugLayer::Init()
{
	_set_error_mode(_OUT_TO_STDERR);
	switch (m_graphicsDebuggerType)
	{
	case GRAPHICS_DEBUGGER_TYPE::PIX:
		m_pixModule = PIXLoadLatestWinPixGpuCapturerLibrary();
		if (!m_pixModule)
		{
			printf("PIX not installed %d\n", GetLastError());
		}
		break;
	case GRAPHICS_DEBUGGER_TYPE::RENDERDOC:
		m_renderdocModule = LoadRenderdoc();
		if (!m_renderdocModule)
		{
			printf("RenderDoc not installed %d\n", GetLastError());
		}
		break;
	default:
		assert(false);
		break;
	}

	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_dxgiDebug)) >> CHK;
	m_dxgiDebug->EnableLeakTrackingForThread();

	D3D12GetDebugInterface(IID_PPV_ARGS(&m_d3d12Debug)) >> CHK;
	m_d3d12Debug->EnableDebugLayer();
	m_d3d12Debug->SetEnableGPUBasedValidation(true);
	m_d3d12Debug->SetEnableAutoName(true);
	m_d3d12Debug->SetEnableSynchronizedCommandQueueValidation(true);
}

void DXDebugLayer::Close()
{
	OutputDebugStringW(L"Report Live Objects:\n");
	m_dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL)) >> CHK;

	if (m_pixModule)
	{
		bool success = FreeLibrary(m_pixModule);
		assert(success);
	}
	if (m_renderdocModule)
	{
		bool success = FreeLibrary(m_renderdocModule);
		assert(success);
	}
}
