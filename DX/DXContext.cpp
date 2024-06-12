#include "../core/Common.h"
#include "DXContext.h"
#include "DXCommon.h"
#include "DXQuery.h"
#include "DXResource.h"

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

DXContext::DXContext() :
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
	ID3D12Device* removed_device = static_cast<ID3D12Device*>(context);
	HRESULT removed_reason = removed_device->GetDeviceRemovedReason();
	std::string removed_reason_string = RemapHResult(removed_reason );
	ASSERT(removed_reason == S_OK);
	LogTrace(removed_reason_string);
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

	CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, m_queue_graphics);
	CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE, m_queue_compute);
	CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY, m_queue_copy);

	m_command_allocator_graphics.resize(g_backbuffer_count);
	// CommandAllocator has to wait that all commands in the command list has been executed by the GPU before reuse
	// CommandList can be reused right away
	for (uint32 i = 0; i < m_command_allocator_graphics.size(); ++i)
		CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_allocator_graphics[i]);
	CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_allocator_graphics[0], m_command_list_graphics);

	CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, m_command_allocator_compute);
	CreateCommandList(D3D12_COMMAND_LIST_TYPE_COMPUTE, m_command_allocator_compute, m_command_list_compute);

	CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, m_command_allocator_copy);
	CreateCommandList(D3D12_COMMAND_LIST_TYPE_COPY, m_command_allocator_copy, m_command_list_copy);
	
	CreateFence(m_fence);

	CreateFence(m_device_removed_fence);
	// On device removal all fences will be set to uint64_max
	m_device_removed_fence.m_gpu->SetEventOnCompletion(UINT64_MAX, m_device_removed_fence.m_event);

	// Callback on event triggered
	// None blocking operation from this thread
	HANDLE wait_handle{};
	bool result = RegisterWaitForSingleObject(
		&wait_handle,
		m_device_removed_fence.m_event,
		OnDeviceRemoved,
		m_device.Get(), // Pass the device as our context
		INFINITE, // No timeout
		0 // No flags
	);
	ASSERT(result);

	NAME_DXGI_OBJECT(m_factory, "Factory");
	NAME_DXGI_OBJECT(m_adapter, "Adapter");

	NAME_DX_OBJECT(m_device, "Device");
	NAME_DX_OBJECT(m_queue_graphics.m_queue, "Queue graphics");
	for (uint32 i = 0; i < m_command_allocator_graphics.size(); ++i)
	{
		std::string str = "Command allocator graphics " + std::to_string(i);
		NAME_DX_OBJECT(m_command_allocator_graphics[i].m_allocator, "Command allocator graphics");
	}
	NAME_DX_OBJECT(m_command_list_graphics.m_list, "Command list graphics");
	NAME_DX_OBJECT(m_queue_compute.m_queue, "Queue compute");
	NAME_DX_OBJECT(m_command_allocator_compute.m_allocator, "Command allocator compute");
	NAME_DX_OBJECT(m_command_list_compute.m_list, "Command list compute");
	NAME_DX_OBJECT(m_queue_copy.m_queue, "Command allocator copy");
	NAME_DX_OBJECT(m_command_allocator_copy.m_allocator, "Command allocator copy");
	NAME_DX_OBJECT(m_command_list_copy.m_list, "Command list copy");
	NAME_DX_OBJECT(m_fence.m_gpu, "Fence");
	NAME_DX_OBJECT(m_device_removed_fence.m_gpu, "Device removed fence");

#if defined(_DEBUG)
	LogTrace(DumpDX12Capabilities(m_device));
#endif
}

// Declaration
extern uint32 g_current_buffer_index;

void DXContext::InitCommandLists()
{
	Wait(m_fence, g_current_buffer_index);
	CommandAllocator& command_allocator = m_command_allocator_graphics[g_current_buffer_index];
	
	if (!m_command_list_graphics.m_is_open)
	{
		command_allocator.m_allocator->Reset() >> CHK;
		m_command_list_graphics.m_list->Reset(command_allocator.m_allocator.Get(), nullptr) >> CHK;
		m_command_list_graphics.m_is_open = true;
	}

	if (!m_command_list_compute.m_is_open)
	{
		m_command_allocator_compute.m_allocator->Reset() >> CHK;
		m_command_list_compute.m_list->Reset(m_command_allocator_compute.m_allocator.Get(), nullptr) >> CHK;
		m_command_list_compute.m_is_open = true;
	}

	if (!m_command_list_copy.m_is_open)
	{
		m_command_allocator_copy.m_allocator->Reset() >> CHK;
		m_command_list_copy.m_list->Reset(m_command_allocator_copy.m_allocator.Get(), nullptr) >> CHK;
		m_command_list_copy.m_is_open = true;
	}
}

void DXContext::ExecuteCommandListGraphics()
{
	m_command_list_graphics.m_list->Close() >> CHK;
	m_command_list_graphics.m_is_open = false;
	ID3D12CommandList* command_lists[] = { m_command_list_graphics.m_list.Get() };
	m_queue_graphics.m_queue->ExecuteCommandLists(COUNT(command_lists), command_lists);
}

void DXContext::ExecuteCommandListCompute()
{
	m_command_list_compute.m_list->Close() >> CHK;
	m_command_list_compute.m_is_open = false;
	ID3D12CommandList* command_lists[] = { m_command_list_compute.m_list.Get() };
	m_queue_compute.m_queue->ExecuteCommandLists(COUNT(command_lists), command_lists);
}

void DXContext::ExecuteCommandListCopy()
{
	m_command_list_copy.m_list->Close() >> CHK;
	m_command_list_copy.m_is_open = false;
	ID3D12CommandList* command_lists[] = { m_command_list_copy.m_list.Get() };
	m_queue_copy.m_queue->ExecuteCommandLists(COUNT(command_lists), command_lists);
}

void DXContext::Signal(const CommandQueue& command_queue, Fence& fence, uint32 index)
{
	++fence.m_value;
	fence.m_cpus[index] = fence.m_value;
	command_queue.m_queue->Signal(fence.m_gpu.Get(), fence.m_value) >> CHK;
}

void DXContext::Wait(const Fence& fence, uint32 index)
{
	fence.m_gpu->SetEventOnCompletion(fence.m_cpus[index], fence.m_event) >> CHK;
	WaitForSingleObject(fence.m_event, INFINITE);
}

void DXContext::SignalAndWait()
{
	Signal(m_queue_graphics, m_fence, g_current_buffer_index);
	Wait(m_fence, g_current_buffer_index);
}

// Wait all GPU operations completed
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

Microsoft::WRL::ComPtr<ID3D12Device14> DXContext::GetDevice() const
{
	return m_device;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> DXContext::GetCommandListGraphics() const
{
	return m_command_list_graphics.m_list;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> DXContext::GetCommandListCompute() const
{
	return m_command_list_compute.m_list;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> DXContext::GetCommandListCopy() const
{
	return m_command_list_copy.m_list;
}

Microsoft::WRL::ComPtr<IDXGIFactory> DXContext::GetFactory() const
{
	return m_factory;
}

CommandQueue DXContext::GetCommandQueue() const
{
	return m_queue_graphics;
}

D3D12_DESCRIPTOR_HEAP_FLAGS GetShaderVisibile(D3D12_DESCRIPTOR_HEAP_TYPE descriptor_heap_type)
{
	// Only CBV / SRV / UAV can be accessed directly in shaders
	// RTV / DSR cant
	// Samplers technically can though?
	return 
		descriptor_heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ?
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
}

void DXContext::CreateDescriptorHeap 
(
	D3D12_DESCRIPTOR_HEAP_TYPE descriptor_heap_type, uint32 number_descriptors,
	DescriptorHeap& out_descriptor_heap
) const
{
	const D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc =
	{
		.Type = descriptor_heap_type,
		.NumDescriptors = number_descriptors,
		.Flags = GetShaderVisibile(descriptor_heap_type),
		.NodeMask = 0,
	};
	m_device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&out_descriptor_heap.m_heap)) >> CHK;
	out_descriptor_heap.m_heap_type = descriptor_heap_type;
	out_descriptor_heap.m_number_descriptors = number_descriptors;
	out_descriptor_heap.m_increment_size = m_device->GetDescriptorHandleIncrementSize(out_descriptor_heap.m_heap_type);

	NAME_DX_OBJECT(out_descriptor_heap.m_heap, "Descriptor Heap");
}

D3D12_CPU_DESCRIPTOR_HANDLE DXContext::GetDescriptorHandle
(
	const DescriptorHeap& descriptor_heap, 
	uint32 i
) const
{
	// Basically pointer to heap
	D3D12_CPU_DESCRIPTOR_HANDLE first_descriptor = descriptor_heap.m_heap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor_handle{};
	descriptor_handle.ptr = first_descriptor.ptr + i * descriptor_heap.m_increment_size;
	return descriptor_handle;
}

void DXContext::CreateRTV
(
	const DXResource& resource, 
	DXGI_FORMAT dxgi_format,
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor_handle
) const
{
	D3D12_RENDER_TARGET_VIEW_DESC rtv_desc
	{
		.Format = dxgi_format,
		.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
		.Texture2D =
		{
			.MipSlice = 0,
			.PlaneSlice = 0,
		},
	};
	m_device->CreateRenderTargetView(resource.m_resource.Get(), &rtv_desc, descriptor_handle);
}

void DXContext::CreateCommandQueue 
(
	D3D12_COMMAND_LIST_TYPE command_queue_type, 
	CommandQueue& out_command_queue)
{
	const D3D12_COMMAND_QUEUE_DESC queue_desc =
	{
		.Type = command_queue_type,
		.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
		.NodeMask = 0,
	};
	out_command_queue.m_type = command_queue_type;
	m_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&out_command_queue.m_queue)) >> CHK;
}

void DXContext::CreateCommandAllocator
(
	D3D12_COMMAND_LIST_TYPE command_allocator_type,
	CommandAllocator& out_command_allocator
)
{
	out_command_allocator.m_type = command_allocator_type;
	m_device->CreateCommandAllocator(out_command_allocator.m_type, IID_PPV_ARGS(&out_command_allocator.m_allocator)) >> CHK;
}

void DXContext::CreateCommandList
(
	D3D12_COMMAND_LIST_TYPE command_list_type, 
	const CommandAllocator& command_allocator,
	CommandList& out_command_list
)
{
	m_device->CreateCommandList(0, command_list_type, command_allocator.m_allocator.Get(), nullptr, IID_PPV_ARGS(&out_command_list.m_list)) >> CHK;
	out_command_list.m_list->Close() >> CHK;
	out_command_list.m_is_open = false;
}

void DXContext::CreateFence(Fence& out_fence)
{
	uint64 initial_value = 0u;
	out_fence.m_value = initial_value;
	memset(&out_fence.m_cpus, (uint32)initial_value, sizeof(out_fence.m_cpus));
	m_device->CreateFence(initial_value  /* Initial fence value */, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&out_fence.m_gpu)) >> CHK;
	out_fence.m_event = CreateEvent(nullptr, false, false, nullptr);
	ASSERT(out_fence.m_event != nullptr);
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
