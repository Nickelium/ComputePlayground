#include "DXContext.h"
#include "DXCommon.h"

DXContext::DXContext() : m_fence_cpu(0u), m_fence_event(0)
{

}

DXContext::~DXContext()
{

}

void DXContext::Init()
{
	CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_factory)) >> CHK;
	m_factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_adapter)) >> CHK;
	DXGI_ADAPTER_DESC1 adapter_desc;
	m_adapter->GetDesc1(&adapter_desc) >> CHK;

	ComPtr<IDXGIOutput> output;
	m_adapter->EnumOutputs(0, output.GetAddressOf()) >> CHK;
	output->QueryInterface(IID_PPV_ARGS(&m_output)) >> CHK;
	DXGI_OUTPUT_DESC1 output_desc;
	m_output->GetDesc1(&output_desc);
	D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)) >> CHK;

	const D3D12_COMMAND_QUEUE_DESC queue_desc =
	{
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
	};
	m_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_command_queue)) >> CHK;
	m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_command_allocator)) >> CHK;
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_allocator.Get(), nullptr, IID_PPV_ARGS(&m_command_list)) >> CHK;
	m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence_gpu)) >> CHK;
	m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	m_factory->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(wchar_t) * _countof(L"Factory"), L"Factory") >> CHK;
	m_adapter->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(wchar_t) * _countof(L"Adapter"), L"Adapter") >> CHK;
	m_output->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(wchar_t) * _countof(L"Adapter"), L"Output") >> CHK;
	m_device->SetName(L"Device") >> CHK;
	m_command_queue->SetName(L"CommandQueue::Direct") >> CHK;
	m_command_allocator->SetName(L"CommandAllocator::Direct") >> CHK;
	m_command_list->SetName(L"CommandList::Direct") >> CHK;
	m_fence_gpu->SetName(L"FenceGPU") >> CHK;

	m_command_list->Close() >> CHK;
}

void DXContext::InitCommandList()
{
	m_command_allocator->Reset() >> CHK;
	m_command_list->Reset(m_command_allocator.Get(), nullptr) >> CHK;
}

void DXContext::ExecuteCommandList()
{
	m_command_list->Close() >> CHK;
	ID3D12CommandList* command_lists[] = { m_command_list.Get() };
	m_command_queue->ExecuteCommandLists(_countof(command_lists), command_lists);
	SignalAndWait();
}

void DXContext::SignalAndWait()
{
	++m_fence_cpu;
	m_command_queue->Signal(m_fence_gpu.Get(), m_fence_cpu) >> CHK;
	m_fence_gpu->SetEventOnCompletion(m_fence_cpu, m_fence_event) >> CHK;
	WaitForSingleObject(m_fence_event, INFINITE);
}

void DXContext::Flush(uint32_t flush_count)
{
	// TODO proper flush
	for (size_t i = 0; i < flush_count; i++)
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
	return m_command_list;
}

ComPtr<IDXGIFactory6> DXContext::GetFactory() const
{
	return m_factory;
}

ComPtr<ID3D12CommandQueue> DXContext::GetCommandQueue() const
{
	return m_command_queue;
}