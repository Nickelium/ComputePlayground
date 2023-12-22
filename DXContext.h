#pragma once
#include "Common.h"
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
	void Flush(uint32_t flushCount);

	ComPtr<ID3D12Device9> GetDevice() const;
	ComPtr<ID3D12GraphicsCommandList6> GetCommandList() const;
	ComPtr<IDXGIFactory6> GetFactory() const;
	ComPtr<ID3D12CommandQueue> GetCommandQueue() const;
protected:
	ComPtr<IDXGIFactory6> m_factory;
	ComPtr<IDXGIAdapter1> m_adapter; // GPU
	ComPtr<IDXGIOutput6> m_output; // Monitor
	ComPtr<ID3D12Device9> m_device;

	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12GraphicsCommandList6> m_commandList;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;

	ComPtr<ID3D12Fence> m_fenceGPU;
	uint64_t m_fenceCPU;
	HANDLE m_fenceEvent;
};

#if defined(_DEBUG)
class DXDebugContext : public DXContext
{
public:
	DXDebugContext(bool loadRenderDoc);

	virtual ~DXDebugContext();

	void Init() override;
private:
	bool m_loadRenderdoc;
	// TODO how to remove includes for these templates
	ComPtr<ID3D12InfoQueue1> m_infoQueue;
	DWORD m_callbackHandle;
	ComPtr<ID3D12DebugDevice2> m_debugDevice;
	ComPtr<ID3D12DebugCommandList1> m_debugCommandList;
	ComPtr<ID3D12DebugCommandQueue> m_debugCommandQueue;
};
#endif