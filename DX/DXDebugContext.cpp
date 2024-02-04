#include "DXDebugContext.h"

DXDebugContext::DXDebugContext(const bool load_renderdoc) : m_load_renderdoc(load_renderdoc), m_callback_handle(0u)
{

}

DXDebugContext::~DXDebugContext()
{
	if (m_info_queue)
		m_info_queue->UnregisterMessageCallback(m_callback_handle) >> CHK;
}

namespace
{
	void CallbackD3D12
	(
		D3D12_MESSAGE_CATEGORY category,
		D3D12_MESSAGE_SEVERITY severity,
		D3D12_MESSAGE_ID id,
		LPCSTR description,
		void* context
	)
	{
		UNUSED(category);
		UNUSED(severity);
		UNUSED(id);
		UNUSED(context);

		std::wstring str = std::to_wstring(description);
		OutputDebugStringW(str.c_str());
		ASSERT(false);
	}
}

void DXDebugContext::Init()
{
	DXContext::Init();

	// RenderDoc doesn't like these
	if (!m_load_renderdoc)
	{
		m_device->QueryInterface(IID_PPV_ARGS(&m_debug_device)) >> CHK;
		m_device->QueryInterface(IID_PPV_ARGS(&m_info_queue)) >> CHK;
		m_info_queue->RegisterMessageCallback(CallbackD3D12, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &m_callback_handle) >> CHK;
		m_command_list->QueryInterface(IID_PPV_ARGS(&m_debug_command_list)) >> CHK;
		m_command_queue->QueryInterface(IID_PPV_ARGS(&m_debug_command_queue)) >> CHK;
	}
}

