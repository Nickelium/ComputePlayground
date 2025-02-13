#pragma once
#include "../core/Common.h"
#include "DXCommon.h"
#include "DXResource.h"
#include "RootSignature.h"
#include "Shader.h"

struct IDXGIFactory6;
struct IDXGIAdapter1;
struct IDXGIOutput6;
struct ID3D12Device9;
struct ID3D12CommandQueue;
struct ID3D12GraphicsCommandList6;
struct ID3D12CommandAllocator;
struct ID3D12Fence;

struct ID3D12InfoQueue1;
struct ID3D12DebugDevice2;
struct ID3D12DebugCommandQueue;

struct ID3D12DescriptorHeap;

class DXResource;

class DXReportContext
{
public:
	DXReportContext() = default;
	~DXReportContext();

	// Keeps device alive to report then free
	void SetDevice(ComPtr<ID3D12Device> device, ComPtr<IDXGIAdapter> adapter);
private:
	// ReportLiveDeviceObjects, ref count 1 is normal since debug_device is the last one
	void ReportLDO();
	ComPtr<ID3D12DebugDevice2> m_debug_device;
	ComPtr<IDXGIAdapter4> m_adapter;
};

// Structure for a single thread to submit
class DXCommand
{
public:
	DXCommand(ComPtr<ID3D12Device> device, const D3D12_COMMAND_LIST_TYPE& command_type);
	~DXCommand();

	void BeginFrame();
	void EndFrame();
private:
	// Command type
	D3D12_COMMAND_LIST_TYPE m_command_type;

	ComPtr<ID3D12CommandQueue> m_command_queue;
	// Number of command list == number of threads, can reuse commandlist after submission
	ComPtr<ID3D12GraphicsCommandList6> m_command_list;
	// Number of command allocator == number of threads x backbuffer count
	ComPtr<ID3D12CommandAllocator> m_command_allocators[g_backbuffer_count];

	uint32 m_frame_index{ 0 };
};

class DXDescriptor;
using SRV = DXDescriptor;
using UAV = DXDescriptor;
using CBV = DXDescriptor;

// Store CPU / GPU descriptor handle from start
struct DescriptorHeap
{
	ComPtr<ID3D12DescriptorHeap> m_heap;
	D3D12_DESCRIPTOR_HEAP_TYPE m_heap_type;
	uint32 m_number_descriptors;
	uint32 m_increment_size;
};

struct CommandQueue
{
	ComPtr<ID3D12CommandQueue> m_queue;
	D3D12_COMMAND_LIST_TYPE m_type;
};

// Esssentially the backing memory of all the commands in the commandlist
struct CommandAllocator
{
	ComPtr<ID3D12CommandAllocator> m_allocator;
	D3D12_COMMAND_LIST_TYPE m_type;
};

struct CommandList
{
	ComPtr<ID3D12GraphicsCommandList10> m_list;
	D3D12_COMMAND_LIST_TYPE m_type;
	bool m_is_open = false;
};

struct Fence
{
	ComPtr<ID3D12Fence> m_gpu;
	uint64 m_cpus[g_backbuffer_count];
	uint64 m_value;
	HANDLE m_event;
};

class DXContext;
// Thin abstraction to avoid all non shader visible descriptors
class RTVDescriptorHandler
{
public:
	void Init(DXContext& dx_context);

	void OMSetRenderTargets
	(
		DXContext& dx_context,
		uint32 num_rtvs,
		ID3D12Resource* const* rtv_resources,
		const D3D12_RENDER_TARGET_VIEW_DESC* rtv_desc,
		ID3D12Resource* dsv_resource,
		const D3D12_DEPTH_STENCIL_VIEW_DESC* dsv_desc
	);
	void ClearRenderTargetView
	(
		DXContext& dx_context,
		ID3D12Resource* rtv_resource,
		const D3D12_RENDER_TARGET_VIEW_DESC* rtv_desc,
		float32 color[4],
		uint32 num_rects,
		const D3D12_RECT* rects
	);
	void ClearDepthStencilView
	(
		DXContext& dx_context,
		ID3D12Resource* dsv_resource,
		const D3D12_DEPTH_STENCIL_VIEW_DESC* dsv_desc,
		D3D12_CLEAR_FLAGS clear_flags,
		float32 depth,
		uint32 stencil,
		uint32 num_rects,
		const D3D12_RECT* rects
	);

private:
	DescriptorHeap m_rtv_descriptor_heap;
	DescriptorHeap m_dsv_descriptor_heap;

	D3D12_CPU_DESCRIPTOR_HANDLE m_rtv_descriptor;
	D3D12_CPU_DESCRIPTOR_HANDLE m_dsv_descriptor;
};

class DXContext
{
public:
	DXContext
	(
		bool enable_debug_layer_cpu, 
		bool enable_debug_layer_gpu, 
		bool enable_dred, 
		bool use_warp
	);
	~DXContext();

	void OMSetRenderTargets
	(
		uint32 num_rtvs,
		ID3D12Resource* const* rtv_resources,
		const D3D12_RENDER_TARGET_VIEW_DESC* rtv_desc,
		ID3D12Resource* dsv_resource,
		const D3D12_DEPTH_STENCIL_VIEW_DESC* dsv_desc
	);
	void ClearRenderTargetView
	(
		ID3D12Resource* rtv_resource,
		const D3D12_RENDER_TARGET_VIEW_DESC* rtv_desc,
		float32 color[4],
		uint32 num_rects,
		const D3D12_RECT* rects
	);
	void ClearDepthStencilView
	(
		ID3D12Resource* dsv_resource,
		const D3D12_DEPTH_STENCIL_VIEW_DESC* dsv_desc,
		D3D12_CLEAR_FLAGS clear_flags,
		float32 depth,
		uint32 stencil,
		uint32 num_rects,
		const D3D12_RECT* rects
	);
 

	void InitCommandLists();
	void ExecuteCommandListGraphics();
	void ExecuteCommandListCompute();
	void ExecuteCommandListCopy();

	void SignalAndWait(uint32 buffer_index);
	void Flush(uint32 flush_count);

	void Transition(D3D12_RESOURCE_STATES new_resource_state, DXResource& resource) const;

	// Note we use the full namespace Microsoft::WRL to help 10xEditor autocompletion
	ComPtr<ID3D12Device14> GetDevice() const;
	ComPtr<ID3D12GraphicsCommandList10> GetCommandListGraphics() const;
	ComPtr<ID3D12GraphicsCommandList>GetCommandListCompute() const;
	ComPtr<ID3D12GraphicsCommandList> GetCommandListCopy() const;
	ComPtr<IDXGIFactory> GetFactory() const;
	CommandQueue GetCommandQueue() const;

	void CreateDescriptorHeap
	(
		D3D12_DESCRIPTOR_HEAP_TYPE descriptor_heap_type, uint32 number_descriptors,
		const std::string& descriptor_name,
		DescriptorHeap& out_descriptor_heap
	) const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle
	(
		const DescriptorHeap& descriptor_heap, 
		uint32 i
	) const;

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle
	(
		const DescriptorHeap& descriptor_heap, 
		uint32 i
	) const;

	void CreateCommandQueue 
	(
		D3D12_COMMAND_LIST_TYPE command_queue_type, 
		CommandQueue& out_command_queue
	);

	void CreateCommandAllocator
	(
		D3D12_COMMAND_LIST_TYPE command_allocator_type, 
		CommandAllocator& out_command_allocator
	);

	void CreateCommandList
	(
		D3D12_COMMAND_LIST_TYPE command_list_type,
		const CommandAllocator& command_allocator,
		CommandList& out_command_list
	);

	void CreateFence(Fence& out_fence);

	void Signal(const CommandQueue& command_queue, Fence& fence, uint32 index);
	void Wait(const Fence& fence, uint32 index);

	RootSignature CreateRS(const Shader& shader) const;

public:
	ComPtr<IDXGIFactory7> m_factory;
	ComPtr<IDXGIAdapter4> m_adapter; // GPU
	ComPtr<ID3D12Device14> m_device;
public:
	// Graphics + Compute + Copy
	CommandQueue m_queue_graphics;
private:
	// Compute + Copy
	// Put work on async in parallel for work that are not bottlenecked by the same limiting factor
	CommandQueue m_queue_compute;
	
	// Copy
	// This is the fastest engine for transfer over PCIe bus
	// Meaning CPU <-> GPU memory transfer
	// For GPU <-> GPU copy, prefer the other engines
	CommandQueue m_queue_copy;
public:
	CommandList m_command_list_graphics;
private:
	std::vector<CommandAllocator> m_command_allocator_graphics;

	CommandList m_command_list_compute;
	CommandAllocator m_command_allocator_compute;

	CommandList m_command_list_copy;
	CommandAllocator m_command_allocator_copy;
public:
	Fence m_fence;
private:
	Fence m_device_removed_fence;
	HANDLE m_device_removed_handle{};

	void CacheDescriptorSizes();
#if defined(_DEBUG)
	DWORD m_callback_handle;
#endif
public:
	// Descriptor size is fixed per GPU
	static uint32 s_descriptor_sizes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	RTVDescriptorHandler m_rtv_descriptor_handler;
	
	void DescriptorAllocateCheck();
	void DescriptorAllocate(D3D12_CPU_DESCRIPTOR_HANDLE& cpu_descriptor, D3D12_GPU_DESCRIPTOR_HANDLE& gpu_descriptor, uint32& bindless_index);
	UAV CreateUAV(const DXResource& resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);
	SRV CreateSRV(const DXResource& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
	CBV CreateCBV(const DXResource& resource);

public:
	std::vector<std::pair<uint64, uint32>> m_list_pair_fence_free_index;
	uint32 m_last_indices[g_backbuffer_count];
	uint32 m_start_index = 0;
	uint32 m_free_index = 0;
	DescriptorHeap m_resources_descriptor_heap;
	DescriptorHeap m_samplers_descriptor_heap;

	ResourceHandler m_resource_handler;
};

inline D3D12_CPU_DESCRIPTOR_HANDLE operator+(D3D12_CPU_DESCRIPTOR_HANDLE x, uint32 y)
{
	return { x.ptr + y };
}

inline D3D12_CPU_DESCRIPTOR_HANDLE operator+(uint32 x, D3D12_CPU_DESCRIPTOR_HANDLE y)
{
	return y + x;
}

// Textures are always typed
// Ex. Use in HLSL: Texture2D<float3> with ex. DXGI_FORMAT_R32G32B32_FLOAT
D3D12_SHADER_RESOURCE_VIEW_DESC GetTexture2DSRVDesc(DXGI_FORMAT format);
// Ex. Use in HLSL: RWTexture2D<unorm float> with ex. R8_UNORM / R16_UNORM
D3D12_UNORDERED_ACCESS_VIEW_DESC GetTexture2DUAVDesc(DXGI_FORMAT format);


// Ex. Use in HLSL: Buffer<float3>
D3D12_SHADER_RESOURCE_VIEW_DESC GetTypedBufferSRVDesc(DXGI_FORMAT format, uint32 number_elements);

// Ex1. Use in HLSL: RWBuffer<float3> with ex. DXGI_FORMAT_R32G32B32_FLOAT
// Ex1. Use in HLSL: RWBuffer<unorm float> with ex. R8_UNORM / R16_UNORM
D3D12_UNORDERED_ACCESS_VIEW_DESC GetTypedBufferUAVDesc(DXGI_FORMAT format, uint32 number_elements);

// Ex. StructuredBuffer<MyStruct>
D3D12_SHADER_RESOURCE_VIEW_DESC GetStructuredBufferSRVDesc(uint32 number_elements, uint32 stride);

// Ex. RWStructuredBuffer<MyStruct>
D3D12_UNORDERED_ACCESS_VIEW_DESC GetStructuredBufferUAVDesc(uint32 number_elements, uint32 stride);

// Ex. ByteAddressBuffer 
D3D12_SHADER_RESOURCE_VIEW_DESC GetByteBufferSRVDesc(uint32 number_bytes);

// Ex. RWByteAddressBuffer
D3D12_UNORDERED_ACCESS_VIEW_DESC GetByteBufferUAVDesc(uint32 number_bytes);
