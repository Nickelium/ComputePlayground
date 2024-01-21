#include "DXDebugLayer.h"
#include "../Common.h"
#include "DXCommon.h"
#include <dxgidebug.h>

#define USE_PIX
#include <pix3.h>

// TODO setup path
#include "C:\Program Files\RenderDoc\renderdoc_app.h"

bool LoadRenderdoc(HMODULE* renderdoc_module_out, RENDERDOC_API_1_6_0** renderdoc_api_out)
{
	RENDERDOC_API_1_6_0* renderdoc_api = nullptr;
	HMODULE renderdoc_module = 0;
	bool success = GetModuleHandleExW(0, L"renderdoc.dll", &renderdoc_module);
	if (!success)
	{
		renderdoc_module = LoadLibraryExW(L"C://Program Files//RenderDoc//renderdoc.dll", NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
		pRENDERDOC_GetAPI renderdoc_get_API =
			(pRENDERDOC_GetAPI)GetProcAddress(renderdoc_module, "RENDERDOC_GetAPI");
		int ret = renderdoc_get_API(eRENDERDOC_API_Version_1_6_0, (void**)&renderdoc_api);
		ASSERT(ret == 1);
		*renderdoc_module_out = renderdoc_module;
		*renderdoc_api_out = renderdoc_api;
	}
	return success;
}

void DXDebugLayer::RenderdocCaptureStart()
{
	if (m_renderdoc_api)
	{
		m_renderdoc_api->SetCaptureFilePathTemplate("captures/rdc");
		m_renderdoc_api->StartFrameCapture(nullptr, nullptr);
	}
}

void DXDebugLayer::RenderdocCaptureEnd()
{
	if (m_renderdoc_api)
		m_renderdoc_api->EndFrameCapture(nullptr, nullptr);
	// TODO search for newest renderdoc file
	// TODO scope out these function calls
	//ShellExecute(0, 0, full_name.c_str(), 0, 0, SW_SHOW);
}

DXDebugLayer::DXDebugLayer(GRAPHICS_DEBUGGER_TYPE type) :
	m_pix_module(0u),
	m_renderdoc_module(0u),
	m_renderdoc_api(nullptr),
	m_graphics_debugger_type(type)
{

}

DXDebugLayer::~DXDebugLayer()
{
	Close();
}

void DXDebugLayer::Init()
{
	_set_error_mode(_OUT_TO_STDERR);
	switch (m_graphics_debugger_type)
	{
	case GRAPHICS_DEBUGGER_TYPE::PIX:
	{
		m_pix_module = PIXLoadLatestWinPixGpuCapturerLibrary();
		if (!m_pix_module)
		{
			printf("PIX not installed %d\n", GetLastError());
		}
		break;
	}
	case GRAPHICS_DEBUGGER_TYPE::RENDERDOC:
	{
		bool success = LoadRenderdoc(&m_renderdoc_module, &m_renderdoc_api);
		if (!success)
		{
			printf("RenderDoc not installed %d\n", GetLastError());
		}
		break;
	}
	default:
	{
		break;
	}
	}

	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_dxgi_debug)) >> CHK;
	m_dxgi_debug->EnableLeakTrackingForThread();

	D3D12GetDebugInterface(IID_PPV_ARGS(&m_d3d12_debug)) >> CHK;
	m_d3d12_debug->EnableDebugLayer();
	m_d3d12_debug->SetEnableGPUBasedValidation(true);
	m_d3d12_debug->SetEnableAutoName(true);
	m_d3d12_debug->SetEnableSynchronizedCommandQueueValidation(true);
}

void DXDebugLayer::Close()
{
	OutputDebugStringW(L"Report Live Objects:\n");
	m_dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL)) >> CHK;

	if (m_pix_module)
	{
		bool success = FreeLibrary(m_pix_module);
		ASSERT(success);
	}
	if (m_renderdoc_module)
	{
		bool success = FreeLibrary(m_renderdoc_module);
		ASSERT(success);
	}
}

#include <filesystem>
void DXDebugLayer::PIXCaptureAndOpen()
{
#if defined(_DEBUG)
	if (m_pix_module)
	{
		std::wstring file_name = L".\\captures\\pix_capture";
		std::wstring extension = L".wpix";
		std::wstring identifier = L"";
		std::wstring full_name = file_name + identifier + extension;
		uint32_t index = 0;
		while (std::filesystem::exists(full_name))
		{
			identifier = L"_" + std::to_wstring(index);
			full_name = file_name + identifier + extension;
			++index;
		}
		PIXGpuCaptureNextFrames(full_name.c_str(), 1) >> CHK;
		ShellExecute(0, 0, full_name.c_str(), 0, 0, SW_SHOW);
	}
#endif
}
