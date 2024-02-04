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

	void InitCommandList();
	void ExecuteCommandList();
	void SignalAndWait();
	void Flush(const uint32_t flush_count);

	ComPtr<ID3D12Device9> GetDevice() const;
	ComPtr<ID3D12GraphicsCommandList6> GetCommandList() const;
	ComPtr<IDXGIFactory6> GetFactory() const;
	ComPtr<ID3D12CommandQueue> GetCommandQueue() const;
protected:
	ComPtr<IDXGIFactory6> m_factory;
	ComPtr<IDXGIAdapter3> m_adapter; // GPU
	ComPtr<IDXGIOutput6> m_output; // Monitor
	ComPtr<ID3D12Device9> m_device;

	ComPtr<ID3D12CommandQueue> m_command_queue;
	ComPtr<ID3D12GraphicsCommandList6> m_command_list;
	ComPtr<ID3D12CommandAllocator> m_command_allocator;

	ComPtr<ID3D12Fence> m_fence_gpu;
	uint64_t m_fence_cpu;
	HANDLE m_fence_event;
};