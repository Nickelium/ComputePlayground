#include "DXContext.h"
#include "DXCommon.h"

DXContext::DXContext() : m_fenceCPU(0u), m_fenceEvent(0)
{

}

DXContext::~DXContext()
{

}

void DXContext::Init()
{
	CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_factory)) >> CHK;
	m_factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_adapter)) >> CHK;
	DXGI_ADAPTER_DESC1 adapterDesc;
	m_adapter->GetDesc1(&adapterDesc) >> CHK;

	ComPtr<IDXGIOutput> output;
	m_adapter->EnumOutputs(0, output.GetAddressOf()) >> CHK;
	output->QueryInterface(IID_PPV_ARGS(&m_output)) >> CHK;
	DXGI_OUTPUT_DESC1 outputDesc;
	m_output->GetDesc1(&outputDesc);
	D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)) >> CHK;

	const D3D12_COMMAND_QUEUE_DESC queueDesc =
	{
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
	};
	m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)) >> CHK;
	m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)) >> CHK;
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)) >> CHK;
	m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fenceGPU)) >> CHK;
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	m_factory->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(wchar_t) * _countof(L"Factory"), L"Factory") >> CHK;
	m_adapter->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(wchar_t) * _countof(L"Adapter"), L"Adapter") >> CHK;
	m_output->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(wchar_t) * _countof(L"Adapter"), L"Output") >> CHK;
	m_device->SetName(L"Device") >> CHK;
	m_commandQueue->SetName(L"CommandQueue::Direct") >> CHK;
	m_commandAllocator->SetName(L"CommandAllocator::Direct") >> CHK;
	m_commandList->SetName(L"CommandList::Direct") >> CHK;
	m_fenceGPU->SetName(L"FenceGPU") >> CHK;

	m_commandList->Close() >> CHK;
}

void DXContext::InitCommandList()
{
	m_commandAllocator->Reset() >> CHK;
	m_commandList->Reset(m_commandAllocator.Get(), nullptr) >> CHK;
}

void DXContext::ExecuteCommandList()
{
	m_commandList->Close() >> CHK;
	ID3D12CommandList* commandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
	SignalAndWait();
}

void DXContext::SignalAndWait()
{
	++m_fenceCPU;
	m_commandQueue->Signal(m_fenceGPU.Get(), m_fenceCPU) >> CHK;
	m_fenceGPU->SetEventOnCompletion(m_fenceCPU, m_fenceEvent) >> CHK;
	WaitForSingleObject(m_fenceEvent, INFINITE);
}

void DXContext::Flush(uint32_t flushCount)
{
	// TODO proper flush
	for (size_t i = 0; i < flushCount; i++)
	{
		SignalAndWait();
	}
}

Microsoft::WRL::ComPtr<ID3D12Device9> DXContext::GetDevice() const
{
	return m_device;
}

ComPtr<ID3D12GraphicsCommandList6> DXContext::GetCommandList() const
{
	return m_commandList;
}

ComPtr<IDXGIFactory6> DXContext::GetFactory() const
{
	return m_factory;
}

ComPtr<ID3D12CommandQueue> DXContext::GetCommandQueue() const
{
	return m_commandQueue;
}

DXDebugContext::DXDebugContext(bool loadRenderDoc) : m_loadRenderdoc(loadRenderDoc), m_callbackHandle(0u)
{

}

DXDebugContext::~DXDebugContext()
{
	if (m_infoQueue)
		m_infoQueue->UnregisterMessageCallback(m_callbackHandle) >> CHK;
}

namespace
{
	void CallbackD3D12
	(
		D3D12_MESSAGE_CATEGORY Category,
		D3D12_MESSAGE_SEVERITY Severity,
		D3D12_MESSAGE_ID ID,
		LPCSTR pDescription,
		void* /*pContext*/
	)
	{
		Category;
		Severity;
		ID;
		pDescription;
		std::wstring str = std::to_wstring(pDescription);
		OutputDebugStringW(str.c_str());
		assert(false);
	}
}

void DXDebugContext::Init()
{
	DXContext::Init();

	// RenderDoc doesn't like these
	if (!m_loadRenderdoc)
	{
		m_device->QueryInterface(IID_PPV_ARGS(&m_debugDevice)) >> CHK;
		m_device->QueryInterface(IID_PPV_ARGS(&m_infoQueue)) >> CHK;
		m_infoQueue->RegisterMessageCallback(CallbackD3D12, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &m_callbackHandle) >> CHK;
		m_commandList->QueryInterface(IID_PPV_ARGS(&m_debugCommandList)) >> CHK;
		m_commandQueue->QueryInterface(IID_PPV_ARGS(&m_debugCommandQueue)) >> CHK;
	}
}
