#include "DXContext.h"
#include "DXCommon.h"

DXContext::DXContext() : m_fence_cpu(0u), m_fence_event(0)
{

}

DXContext::~DXContext()
{

}

D3D_FEATURE_LEVEL GetMaxFeatureLevel(ComPtr<IDXGIAdapter4> adapter)
{
	// All D3D12 compliant feature levels
	const D3D_FEATURE_LEVEL feature_levels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_2
	};

	// Require to support atleast bare d3d12
	const D3D_FEATURE_LEVEL min_feature_level = feature_levels[0];
	ComPtr<ID3D12Device9> device{};
	D3D12CreateDevice(adapter.Get(), min_feature_level, IID_PPV_ARGS(&device)) >> CHK;

	const D3D12_FEATURE feature{ D3D12_FEATURE_FEATURE_LEVELS };
	D3D12_FEATURE_DATA_FEATURE_LEVELS feature_data =
	{
		.NumFeatureLevels = countof(feature_levels),
		.pFeatureLevelsRequested = feature_levels,
	};
	device->CheckFeatureSupport(feature, &feature_data, sizeof(feature_data)) >> CHK;
	return feature_data.MaxSupportedFeatureLevel;
}

void DXContext::Init()
{
	uint32 dxgi_factory_flag{ 0 };
#if defined(_DEBUG)
	dxgi_factory_flag |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	CreateDXGIFactory2(dxgi_factory_flag, IID_PPV_ARGS(&m_factory)) >> CHK;
	m_factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_adapter)) >> CHK;
	DXGI_ADAPTER_DESC1 adapter_desc{};
	m_adapter->GetDesc1(&adapter_desc) >> CHK;

	// node_index 0 because single GPU
	const uint32 node_index{0};
	// Local means non-system main memory, non CPU RAM, aka VRAM
	const DXGI_MEMORY_SEGMENT_GROUP memory_segment_group{ DXGI_MEMORY_SEGMENT_GROUP_LOCAL};
	DXGI_QUERY_VIDEO_MEMORY_INFO video_memory_info{};
	m_adapter->QueryVideoMemoryInfo(node_index, memory_segment_group, &video_memory_info) >> CHK;
	//video_memory_info.Budget >> 30;

	ComPtr<IDXGIOutput> output{};
	m_adapter->EnumOutputs(0, output.GetAddressOf()) >> CHK;
	output->QueryInterface(IID_PPV_ARGS(&m_output)) >> CHK;
	DXGI_OUTPUT_DESC1 output_desc{};
	m_output->GetDesc1(&output_desc);

	D3D_FEATURE_LEVEL max_feature_level = GetMaxFeatureLevel(m_adapter);
	D3D12CreateDevice(m_adapter.Get(), max_feature_level, IID_PPV_ARGS(&m_device)) >> CHK;
	
	const D3D12_COMMAND_QUEUE_DESC graphics_queue_desc =
	{
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
	};
	m_device->CreateCommandQueue(&graphics_queue_desc, IID_PPV_ARGS(&m_graphics_queue)) >> CHK;
	const D3D12_COMMAND_QUEUE_DESC compute_queue_desc =
	{
		.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE,
		.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
	};
	m_device->CreateCommandQueue(&compute_queue_desc, IID_PPV_ARGS(&m_compute_queue)) >> CHK;
	const D3D12_COMMAND_QUEUE_DESC copy_queue_desc =
	{
		.Type = D3D12_COMMAND_LIST_TYPE_COPY,
		.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
	};
	m_device->CreateCommandQueue(&copy_queue_desc, IID_PPV_ARGS(&m_copy_queue)) >> CHK;

	m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_command_allocator_graphics)) >> CHK;
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_allocator_graphics.Get(), nullptr, IID_PPV_ARGS(&m_command_list_graphics)) >> CHK;
	
	m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&m_command_allocator_compute)) >> CHK;
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_command_allocator_compute.Get(), nullptr, IID_PPV_ARGS(&m_command_list_compute)) >> CHK;

	m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_command_allocator_copy)) >> CHK;
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, m_command_allocator_copy.Get(), nullptr, IID_PPV_ARGS(&m_command_list_copy)) >> CHK;

	m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence_gpu)) >> CHK;
	m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

#if defined(_DEBUG)
	m_factory->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(wchar_t) * _countof(L"Factory"), L"Factory") >> CHK;
	m_adapter->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(wchar_t) * _countof(L"Adapter"), L"Adapter") >> CHK;
	m_output->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(wchar_t) * _countof(L"Adapter"), L"Output") >> CHK;

	m_device->SetName(L"Device") >> CHK;
	m_graphics_queue->SetName(L"CommandQueue::Direct") >> CHK;
	m_command_allocator_graphics->SetName(L"CommandAllocator::Direct") >> CHK;
	m_command_list_graphics->SetName(L"CommandList::Direct") >> CHK;
	m_command_allocator_compute->SetName(L"CommandAllocator::Compute") >> CHK;
	m_command_list_compute->SetName(L"CommandList::Compute") >> CHK;
	m_command_allocator_copy->SetName(L"CommandAllocator::Copy") >> CHK;
	m_command_list_copy->SetName(L"CommandList::Copy") >> CHK;
	m_fence_gpu->SetName(L"FenceGPU") >> CHK;
#endif

	m_command_list_graphics->Close() >> CHK;
	m_is_graphics_command_list_open = false;
	m_command_list_compute->Close() >> CHK;
	m_is_compute_command_list_open = false;
	m_command_list_copy->Close() >> CHK;
	m_is_copy_command_list_open = false;
}

void DXContext::InitCommandLists()
{
	if (!m_is_graphics_command_list_open)
	{
		m_command_allocator_graphics->Reset() >> CHK;
		m_command_list_graphics->Reset(m_command_allocator_graphics.Get(), nullptr) >> CHK;
		m_is_graphics_command_list_open = true;
	}

	if (!m_is_compute_command_list_open)
	{
		m_command_allocator_compute->Reset() >> CHK;
		m_command_list_compute->Reset(m_command_allocator_compute.Get(), nullptr) >> CHK;
		m_is_compute_command_list_open = true;
	}

	if (!m_is_copy_command_list_open)
	{
		m_command_allocator_copy->Reset() >> CHK;
		m_command_list_copy->Reset(m_command_allocator_copy.Get(), nullptr) >> CHK;
		m_is_copy_command_list_open = true;
	}
}

void DXContext::ExecuteCommandListGraphics()
{
	m_command_list_graphics->Close() >> CHK;
	m_is_graphics_command_list_open = false;
	ID3D12CommandList* command_lists[] = { m_command_list_graphics.Get() };
	m_graphics_queue->ExecuteCommandLists(_countof(command_lists), command_lists);
	SignalAndWait();
}

void DXContext::ExecuteCommandListCompute()
{
	m_command_list_compute->Close() >> CHK;
	m_is_compute_command_list_open = false;
	ID3D12CommandList* command_lists[] = { m_command_list_compute.Get() };
	m_compute_queue->ExecuteCommandLists(_countof(command_lists), command_lists);
	SignalAndWait();
}

void DXContext::ExecuteCommandListCopy()
{
	m_command_list_copy->Close() >> CHK;
	m_is_copy_command_list_open = false;
	ID3D12CommandList* command_lists[] = { m_command_list_copy.Get() };
	m_copy_queue->ExecuteCommandLists(_countof(command_lists), command_lists);
	SignalAndWait();
}

void DXContext::SignalAndWait()
{
	++m_fence_cpu;
	m_graphics_queue->Signal(m_fence_gpu.Get(), m_fence_cpu) >> CHK;
	m_fence_gpu->SetEventOnCompletion(m_fence_cpu, m_fence_event) >> CHK;
	WaitForSingleObject(m_fence_event, INFINITE);
}

void DXContext::Flush(const uint32_t flush_count)
{
	// TODO proper flush
	for (size_t i = 0; i < flush_count; i++)
	{
		SignalAndWait();
	}
}

ComPtr<ID3D12Device9> DXContext::GetDevice() const
{
	return m_device;
}

ComPtr<ID3D12GraphicsCommandList6> DXContext::GetCommandListGraphics() const
{
	return m_command_list_graphics;
}

ComPtr<ID3D12GraphicsCommandList6> DXContext::GetCommandListCompute() const
{
	return m_command_list_compute;
}

ComPtr<ID3D12GraphicsCommandList6> DXContext::GetCommandListCopy() const
{
	return m_command_list_copy;
}

ComPtr<IDXGIFactory7> DXContext::GetFactory() const
{
	return m_factory;
}

ComPtr<ID3D12CommandQueue> DXContext::GetCommandQueue() const
{
	return m_graphics_queue;
}