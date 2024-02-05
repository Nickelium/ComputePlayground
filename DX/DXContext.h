#pragma once
#include "../Common.h"
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

class DXContext
{
public:
	DXContext();
	virtual ~DXContext();
	virtual void Init();

	void InitCommandLists();
	void ExecuteCommandListGraphics();
	void ExecuteCommandListCompute();
	void ExecuteCommandListCopy();

	void SignalAndWait();
	void Flush(const uint32_t flush_count);

	ComPtr<ID3D12Device9> GetDevice() const;
	ComPtr<ID3D12GraphicsCommandList6> GetCommandListGraphics() const;
	ComPtr<ID3D12GraphicsCommandList6> GetCommandListCompute() const;
	ComPtr<ID3D12GraphicsCommandList6> GetCommandListCopy() const;
	ComPtr<IDXGIFactory7> GetFactory() const;
	ComPtr<ID3D12CommandQueue> GetCommandQueue() const;
protected:
	ComPtr<IDXGIFactory7> m_factory;
	ComPtr<IDXGIAdapter4> m_adapter; // GPU
	ComPtr<IDXGIOutput6> m_output; // Monitor
	ComPtr<ID3D12Device9> m_device;


	// Graphics + Compute + Copy
	ComPtr<ID3D12CommandQueue> m_graphics_queue;
	// Compute + Copy
	ComPtr<ID3D12CommandQueue> m_compute_queue;
	// Copy
	ComPtr<ID3D12CommandQueue> m_copy_queue;
	
	bool m_is_graphics_command_list_open = false;
	ComPtr<ID3D12GraphicsCommandList6> m_command_list_graphics;
	ComPtr<ID3D12CommandAllocator> m_command_allocator_graphics;

	bool m_is_compute_command_list_open = false;
	ComPtr<ID3D12GraphicsCommandList6> m_command_list_compute;
	ComPtr<ID3D12CommandAllocator> m_command_allocator_compute;

	bool m_is_copy_command_list_open = false;
	ComPtr<ID3D12GraphicsCommandList6> m_command_list_copy;
	ComPtr<ID3D12CommandAllocator> m_command_allocator_copy;

	ComPtr<ID3D12Fence> m_fence_gpu;
	uint64_t m_fence_cpu;
	HANDLE m_fence_event;
};