#include "../core/Common.h"
#include "DXContext.h"
#include "DXCommon.h"
#include "DXQuery.h"
#include "DXResource.h"

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

DXContext::DXContext() :
	m_fence_cpu(0u), 
	m_fence_event(0),
	m_use_warp(true)
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
	Microsoft::WRL::ComPtr<ID3D12InfoQueue1> info_queue{};
	HRESULT result = m_device.As(&info_queue);
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
		Microsoft::WRL::ComPtr<IDXGIDebug1> dxgi_debug{};
		// Requires windows "Graphics Tool" optional feature
		DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug)) >> CHK;
		dxgi_debug->EnableLeakTrackingForThread();
	}

	{
		Microsoft::WRL::ComPtr<ID3D12Debug5> d3d12_debug{};
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

	if (m_use_warp)
	{
		m_factory->EnumWarpAdapter(IID_PPV_ARGS(&m_adapter)) >> CHK;
	}
	else
	{
		m_factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_adapter)) >> CHK;
	}

	DXGI_ADAPTER_DESC2 adapter_desc;
	m_adapter->GetDesc2(&adapter_desc) >> CHK;
	LogTrace(std::to_string(adapter_desc.Description));

	// node_index 0 because single GPU
	const uint32 node_index{0};
	// Local means non-system main memory, non CPU RAM, aka VRAM
	const DXGI_MEMORY_SEGMENT_GROUP memory_segment_group{ DXGI_MEMORY_SEGMENT_GROUP_LOCAL};
	DXGI_QUERY_VIDEO_MEMORY_INFO video_memory_info{};
	m_adapter->QueryVideoMemoryInfo(node_index, memory_segment_group, &video_memory_info) >> CHK;
	//video_memory_info.Budget >> 30;

	D3D_FEATURE_LEVEL max_feature_level = GetMaxFeatureLevel(m_adapter);
	D3D12CreateDevice(m_adapter.Get(), max_feature_level, IID_PPV_ARGS(&m_device)) >> CHK;

#if defined(_DEBUG)
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue1> info_queue{};
		HRESULT result = m_device.As(&info_queue);
		// Query fails if debug layer disabled
		if (result == S_OK)
		{
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true) >> CHK;
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true) >> CHK;
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true) >> CHK;
			info_queue->RegisterMessageCallback(CallbackD3D12, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &m_callback_handle) >> CHK;
		
			// D3D12_MESSAGE_CATEGORY allow_category_list[] = {};
			// D3D12_MESSAGE_SEVERITY allow_severity_list[] = {};
			// D3D12_MESSAGE_ID allow_id_list[] = {};

			// D3D12_MESSAGE_CATEGORY deny_category_list[] = {};
			D3D12_MESSAGE_SEVERITY deny_severity_list[] = 
			{
				D3D12_MESSAGE_SEVERITY_INFO, 
				//D3D12_MESSAGE_SEVERITY_MESSAGE 
			};
			// D3D12_MESSAGE_ID deny_id_list[] = {};

			D3D12_INFO_QUEUE_FILTER filter{};
			filter.AllowList =
			{
				.NumCategories = 0,
				.pCategoryList = nullptr,
				.NumSeverities = 0,
				.pSeverityList = nullptr,
				.NumIDs = 0,
				.pIDList = nullptr,
			};
			filter.DenyList =
			{
				.NumCategories = 0,
				.pCategoryList = nullptr,
				.NumSeverities = COUNT(deny_severity_list),
				.pSeverityList = deny_severity_list,
				.NumIDs = 0,
				.pIDList = nullptr,
			};
			info_queue->PushStorageFilter(&filter) >> CHK;
		}
	}
#endif


	auto CreateCommandQueue = [](Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE command_queue_type, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& out_command_queue)
	{
		const D3D12_COMMAND_QUEUE_DESC queue_desc =
		{
			.Type = command_queue_type,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0,
		};
		device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&out_command_queue)) >> CHK;
	};

	CreateCommandQueue(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT, m_queue_graphics);
	CreateCommandQueue(m_device, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_queue_compute);
	CreateCommandQueue(m_device, D3D12_COMMAND_LIST_TYPE_COPY, m_queue_copy);

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

void DXContext::Transition(D3D12_RESOURCE_STATES new_resource_state, DXResource& resource) const
{
	if (new_resource_state != resource.m_resource_state)
	{
		D3D12_RESOURCE_BARRIER barrier[1]{};
		barrier[0] =
		{
			.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
			.Transition =
			{
				.pResource = resource.m_resource.Get(),
				.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				.StateBefore = resource.m_resource_state,
				.StateAfter = new_resource_state,
			}
		};
		GetCommandListGraphics()->ResourceBarrier(COUNT(barrier), barrier);
		resource.m_resource_state = new_resource_state;
	}
}

Microsoft::WRL::ComPtr<ID3D12Device> DXContext::GetDevice() const
{
	return m_device;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> DXContext::GetCommandListGraphics() const
{
	return m_command_list_graphics;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> DXContext::GetCommandListCompute() const
{
	return m_command_list_compute;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> DXContext::GetCommandListCopy() const
{
	return m_command_list_copy;
}

Microsoft::WRL::ComPtr<IDXGIFactory> DXContext::GetFactory() const
{
	return m_factory;
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> DXContext::GetCommandQueue() const
{
	return m_queue_graphics;
}

DXReportContext::~DXReportContext()
{
	ReportLDO();
}

void DXReportContext::SetDevice(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	// Query fails if debug layer disabled
	HRESULT result = device.As(&m_debug_device);
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
		Microsoft::WRL::ComPtr<IDXGIDebug1> dxgi_debug{};
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
	{ D3D12_COMMAND_LIST_TYPE_DIRECT, "Graphics"},
	{ D3D12_COMMAND_LIST_TYPE_COMPUTE, "Compute"},
	{ D3D12_COMMAND_LIST_TYPE_COPY, "Copy"}
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

DXCommand::DXCommand(Microsoft::WRL::ComPtr<ID3D12Device> device, const D3D12_COMMAND_LIST_TYPE& command_type)
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
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator = m_command_allocators[m_frame_index];
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
