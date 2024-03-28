#include "../core/Common.h"
#include "DXContext.h"
#include "DXCommon.h"
#include "DXQuery.h"

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

DXContext::DXContext() :
	m_fence_cpu(0u), 
	m_fence_event(0)
#if defined(_DEBUG)
	,
	m_callback_handle(0)
#endif
{
	Init();
}

DXContext::~DXContext()
{
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue1> info_queue{};
	HRESULT result = m_device->QueryInterface(IID_PPV_ARGS(&info_queue));
	// Query fails if debug layer disabled
	if (result == S_OK)
	{
		// Reset back breaks otherwise breaks in ReportLiveDeviceObjects
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false) >> CHK;
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false) >> CHK;
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false) >> CHK;
		if (m_callback_handle != 0)
		{
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

void OnDeviceRemoved(PVOID context, BOOLEAN)
{
	ID3D12Device* removedDevice = (ID3D12Device*)context;
	HRESULT removedReason = removedDevice->GetDeviceRemovedReason();
	std::string removed_reason_string = RemapHResult(removedReason);
	// Perform app-specific device removed operation, such as logging or inspecting DRED output
	ASSERT(removedReason == S_OK);
}

void DXContext::Init()
{
#if defined(_DEBUG)
	{
		ComPtr<IDXGIDebug1> dxgi_debug{};
		// Requires windows "Graphics Tool" optional feature
		DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug)) >> CHK;
		dxgi_debug->EnableLeakTrackingForThread();
	}

	{
		ComPtr<ID3D12Debug5> d3d12_debug{};
		D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12_debug)) >> CHK;
		d3d12_debug->EnableDebugLayer();
		d3d12_debug->SetEnableGPUBasedValidation(true);
		//d3d12_debug->SetEnableAutoName(true);
		d3d12_debug->SetEnableSynchronizedCommandQueueValidation(true);
	}
#endif

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

	HANDLE deviceRemovedEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
	ASSERT(deviceRemovedEvent != NULL);
	m_fence_gpu->SetEventOnCompletion(UINT64_MAX, deviceRemovedEvent);

	HANDLE waitHandle;
	RegisterWaitForSingleObject(
		&waitHandle,
		deviceRemovedEvent,
		OnDeviceRemoved,
		m_device.Get(), // Pass the device as our context
		INFINITE, // No timeout
		0 // No flags
	);

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

	LogTrace(DumpDX12Capabilities(m_device));
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
	m_queue_graphics->ExecuteCommandLists(COUNT(command_lists), command_lists);
	SignalAndWait();
}

void DXContext::ExecuteCommandListCompute()
{
	m_command_list_compute->Close() >> CHK;
	m_is_compute_command_list_open = false;
	ID3D12CommandList* command_lists[] = { m_command_list_compute.Get() };
	m_queue_compute->ExecuteCommandLists(COUNT(command_lists), command_lists);
	SignalAndWait();
}

void DXContext::ExecuteCommandListCopy()
{
	m_command_list_copy->Close() >> CHK;
	m_is_copy_command_list_open = false;
	ID3D12CommandList* command_lists[] = { m_command_list_copy.Get() };
	m_queue_copy->ExecuteCommandLists(COUNT(command_lists), command_lists);
	SignalAndWait();
}

void DXContext::SignalAndWait()
{
	++m_fence_cpu;
	m_queue_graphics->Signal(m_fence_gpu.Get(), m_fence_cpu) >> CHK;
	m_fence_gpu->SetEventOnCompletion(m_fence_cpu, m_fence_event) >> CHK;
	WaitForSingleObject(m_fence_event, INFINITE);
}

void DXContext::Flush(uint32 flush_count)
{
	for (uint32 i = 0; i < flush_count; ++i)
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
#if defined(_DEBUG)
	if (m_debug_device)
	{
		OutputDebugStringW(std::to_wstring("Report Live D3D12 Objects:\n").c_str());
		m_debug_device->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL) >> CHK;
	}

	{
		ComPtr<IDXGIDebug1> dxgi_debug{};
		// Requires windows "Graphics Tool" optional feature
		DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug)) >> CHK;
		OutputDebugStringW(std::to_wstring("Report Live DXGI Objects:\n").c_str());
		dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL)) >> CHK;
	}
#endif
}

#include <map>

static const std::map<D3D12_COMMAND_LIST_TYPE, std::string> g_command_list_type_map_string =
{
	{D3D12_COMMAND_LIST_TYPE_DIRECT, "Graphics"},
	{D3D12_COMMAND_LIST_TYPE_COMPUTE, "Compute"},
	{D3D12_COMMAND_LIST_TYPE_COPY, "Copy"}
};

std::string GetCommandTypeToString(const D3D12_COMMAND_LIST_TYPE& command_type)
{
	const auto& iterator = g_command_list_type_map_string.find(command_type);
	if (iterator != g_command_list_type_map_string.cend())
	{
		return iterator->second;
	}
	ASSERT(false && "Command type invalid or not supported");
	return "";
}

DXCommand::DXCommand(ComPtr<ID3D12Device> device, const D3D12_COMMAND_LIST_TYPE& command_type)
	: m_command_type(command_type)
{
	const D3D12_COMMAND_QUEUE_DESC queue_desc =
	{
		.Type = m_command_type,
		.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
	};
	device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_command_queue)) >> CHK;
	for (uint32 i = 0; i < g_backbuffer_count; ++i)
	{
		device->CreateCommandAllocator(command_type, IID_PPV_ARGS(&m_command_allocators[i])) >> CHK;
	}
	
	device->CreateCommandList(0, command_type, m_command_allocators[m_frame_index].Get(), nullptr, IID_PPV_ARGS(&m_command_list)) >> CHK;

	NAME_DX_OBJECT(m_command_queue, "Command Queue: " + GetCommandTypeToString(m_command_type));
	for (uint32 i = 0; i < g_backbuffer_count; ++i)
	{
		NAME_DX_OBJECT(m_command_allocators[i], "Command Allocator: " + GetCommandTypeToString(m_command_type) + std::to_string(i));
	}
	NAME_DX_OBJECT(m_command_list, "Command List: " + GetCommandTypeToString(m_command_type));
}

DXCommand::~DXCommand()
{

}

void DXCommand::BeginFrame()
{
	ComPtr<ID3D12CommandAllocator> command_allocator = m_command_allocators[m_frame_index];
	// Wait GPU using command allocator
	// Free command allocator
	command_allocator->Reset() >> CHK;
	// Free commandlist
	m_command_list->Reset(command_allocator.Get(), nullptr) >> CHK;
	// Command recording
	// ..
}

void DXCommand::EndFrame()
{
	// Close commandlist
	m_command_list->Close() >> CHK;
	// Submit commandlist
	ID3D12CommandList* command_lists[] =
	{
		m_command_list.Get()
	};
	m_command_queue->ExecuteCommandLists(COUNT(command_lists), command_lists);

	m_frame_index = (m_frame_index + 1) % g_backbuffer_count;
}
