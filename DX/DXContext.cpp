#include "DXContext.h"
#include "DXCommon.h"
#include "DXQuery.h"


DXContext::DXContext(const bool load_renderdoc) :
	m_fence_cpu(0u), 
	m_fence_event(0)
#if defined(_DEBUG)
	,
	m_load_renderdoc(load_renderdoc),
	m_callback_handle(0)
#endif
{
#if !defined(_DEBUG)
	UNUSED(load_renderdoc);
#endif
}

DXContext::~DXContext()
{
#if defined(_DEBUG)
	if (!m_load_renderdoc && m_callback_handle != 0)
	{
		ComPtr<ID3D12InfoQueue1> info_queue{};
		HRESULT result = m_device->QueryInterface(IID_PPV_ARGS(&info_queue));
		// Query fails if debug layer disabled
		if (result == S_OK)
		{
			// Reset back breaks otherwise breaks in ReportLiveDeviceObjects
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false) >> CHK;
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false) >> CHK;
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false) >> CHK;
			info_queue->UnregisterMessageCallback(m_callback_handle) >> CHK;
		}
	}
#endif
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
	m_device->CreateCommandQueue(&graphics_queue_desc, IID_PPV_ARGS(&m_queue_graphics)) >> CHK;
	const D3D12_COMMAND_QUEUE_DESC compute_queue_desc =
	{
		.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE,
		.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
	};
	m_device->CreateCommandQueue(&compute_queue_desc, IID_PPV_ARGS(&m_queue_compute)) >> CHK;
	const D3D12_COMMAND_QUEUE_DESC copy_queue_desc =
	{
		.Type = D3D12_COMMAND_LIST_TYPE_COPY,
		.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
	};
	m_device->CreateCommandQueue(&copy_queue_desc, IID_PPV_ARGS(&m_queue_copy)) >> CHK;

	m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_command_allocator_graphics)) >> CHK;
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_allocator_graphics.Get(), nullptr, IID_PPV_ARGS(&m_command_list_graphics)) >> CHK;
	
	m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&m_command_allocator_compute)) >> CHK;
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_command_allocator_compute.Get(), nullptr, IID_PPV_ARGS(&m_command_list_compute)) >> CHK;

	m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_command_allocator_copy)) >> CHK;
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, m_command_allocator_copy.Get(), nullptr, IID_PPV_ARGS(&m_command_list_copy)) >> CHK;

	m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence_gpu)) >> CHK;
	m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	NAME_DXGI_OBJECT(m_factory, "Factory");
	NAME_DXGI_OBJECT(m_adapter, "Adapter");
	NAME_DXGI_OBJECT(m_output, "Output");

	NAME_DX_OBJECT(m_device, "Device");
	NAME_DX_OBJECT(m_queue_graphics, "Queue graphics");
	NAME_DX_OBJECT(m_command_allocator_graphics, "Command allocator graphics");
	NAME_DX_OBJECT(m_command_list_graphics, "Command list graphics");
	NAME_DX_OBJECT(m_queue_compute, "Queue compute");
	NAME_DX_OBJECT(m_command_allocator_compute, "Command allocator compute");
	NAME_DX_OBJECT(m_command_list_compute, "Command list compute");
	NAME_DX_OBJECT(m_queue_copy, "Command allocator copy");
	NAME_DX_OBJECT(m_command_allocator_copy, "Command allocator copy");
	NAME_DX_OBJECT(m_command_list_copy, "Command list copy");
	NAME_DX_OBJECT(m_fence_gpu, "FenceGPU");

	m_command_list_graphics->Close() >> CHK;
	m_is_graphics_command_list_open = false;
	m_command_list_compute->Close() >> CHK;
	m_is_compute_command_list_open = false;
	m_command_list_copy->Close() >> CHK;
	m_is_copy_command_list_open = false;

#if defined(_DEBUG)
	if (!m_load_renderdoc)
	{
		ComPtr<ID3D12InfoQueue1> info_queue{};
		HRESULT result = m_device->QueryInterface(IID_PPV_ARGS(&info_queue));
		// Query fails if debug layer disabled
		if (result == S_OK)
		{
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true) >> CHK;
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true) >> CHK;
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true) >> CHK;
			info_queue->RegisterMessageCallback(CallbackD3D12, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &m_callback_handle) >> CHK;
		}
	}

	printf("%s", DumpDX12Capabilities(m_device).c_str());
#endif
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
	m_queue_graphics->ExecuteCommandLists(countof(command_lists), command_lists);
	SignalAndWait();
}

void DXContext::ExecuteCommandListCompute()
{
	m_command_list_compute->Close() >> CHK;
	m_is_compute_command_list_open = false;
	ID3D12CommandList* command_lists[] = { m_command_list_compute.Get() };
	m_queue_compute->ExecuteCommandLists(countof(command_lists), command_lists);
	SignalAndWait();
}

void DXContext::ExecuteCommandListCopy()
{
	m_command_list_copy->Close() >> CHK;
	m_is_copy_command_list_open = false;
	ID3D12CommandList* command_lists[] = { m_command_list_copy.Get() };
	m_queue_copy->ExecuteCommandLists(countof(command_lists), command_lists);
	SignalAndWait();
}

void DXContext::SignalAndWait()
{
	++m_fence_cpu;
	m_queue_graphics->Signal(m_fence_gpu.Get(), m_fence_cpu) >> CHK;
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

ComPtr<ID3D12Device> DXContext::GetDevice() const
{
	return m_device;
}

ComPtr<ID3D12GraphicsCommandList> DXContext::GetCommandListGraphics() const
{
	return m_command_list_graphics;
}

ComPtr<ID3D12GraphicsCommandList> DXContext::GetCommandListCompute() const
{
	return m_command_list_compute;
}

ComPtr<ID3D12GraphicsCommandList> DXContext::GetCommandListCopy() const
{
	return m_command_list_copy;
}

ComPtr<IDXGIFactory> DXContext::GetFactory() const
{
	return m_factory;
}

ComPtr<ID3D12CommandQueue> DXContext::GetCommandQueue() const
{
	return m_queue_graphics;
}

DXReportContext::~DXReportContext()
{
	ReportLDO();
}

void DXReportContext::SetDevice(ComPtr<ID3D12Device> device)
{
	// Query fails if debug layer disabled
	HRESULT result = device->QueryInterface(IID_PPV_ARGS(&m_debug_device));
	UNUSED(result);
}

void DXReportContext::ReportLDO() const
{
	if (m_debug_device)
	{
		OutputDebugStringW(std::to_wstring("Report Live D3D12 Objects:\n").c_str());
		m_debug_device->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL) >> CHK;
	}
}
