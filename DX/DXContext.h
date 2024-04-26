#pragma once
#include "../core/Common.h"
#include "DXCommon.h"

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
struct ID3D12DebugCommandList1;
struct ID3D12DebugCommandQueue;

class DXResource;

class DXReportContext
{
public:
	DXReportContext() = default;
	~DXReportContext();

	// Keeps device alive to report then free
	void SetDevice(Microsoft::WRL::ComPtr<ID3D12Device> device);
private:
	// ReportLiveDeviceObjects, ref count 1 is normal since debug_device is the last one
	void ReportLDO() const;
	Microsoft::WRL::ComPtr<ID3D12DebugDevice2> m_debug_device;
};

// Structure for a single thread to submit
class DXCommand
{
public:
	DXCommand(Microsoft::WRL::ComPtr<ID3D12Device> device, const D3D12_COMMAND_LIST_TYPE& command_type);
	~DXCommand();

	void BeginFrame();
	void EndFrame();
private:
	// Command type
	D3D12_COMMAND_LIST_TYPE m_command_type;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_command_queue;
	// Number of command list == number of threads, can reuse commandlist after submission
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> m_command_list;
	// Number of command allocator == number of threads x backbuffer count
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_command_allocators[g_backbuffer_count];

	uint32 m_frame_index{ 0 };
};


class DXContext
{
public:
	DXContext();
	~DXContext();

	void InitCommandLists();
	void ExecuteCommandListGraphics();
	void ExecuteCommandListCompute();
	void ExecuteCommandListCopy();

	void SignalAndWait();
	void Flush(uint32 flush_count);

	void Transition(D3D12_RESOURCE_STATES new_resource_state, DXResource& resource) const;

	// Note we use the full namespace Microsoft::WRL to help 10xEditor autocompletion
	Microsoft::WRL::ComPtr<ID3D12Device> GetDevice() const;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandListGraphics() const;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandListCompute() const;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandListCopy() const;
	Microsoft::WRL::ComPtr<IDXGIFactory> GetFactory() const;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const;
private:
	void Init();

	Microsoft::WRL::ComPtr<IDXGIFactory7> m_factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_adapter; // GPU
	Microsoft::WRL::ComPtr<ID3D12Device9> m_device;
	bool m_use_warp;

	// Graphics + Compute + Copy
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_queue_graphics;
	// Compute + Copy
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_queue_compute;
	// Copy
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_queue_copy;
	
	bool m_is_graphics_command_list_open = false;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> m_command_list_graphics;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_command_allocator_graphics;

	bool m_is_compute_command_list_open = false;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> m_command_list_compute;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_command_allocator_compute;

	bool m_is_copy_command_list_open = false;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> m_command_list_copy;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_command_allocator_copy;

	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence_gpu;
	uint64_t m_fence_cpu;
	HANDLE m_fence_event;

#if defined(_DEBUG)
	DWORD m_callback_handle;
#endif
};