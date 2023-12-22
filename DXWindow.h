#pragma once
#include "Common.h"

class DXContext;

struct IDXGISwapChain4;
struct ID3D12Resource2;
struct ID3D12DescriptorHeap;
struct D3D12_CPU_DESCRIPTOR_HANDLE;

class DXWindow
{
public:
	static LRESULT CALLBACK OnWindowMessage(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam);

	void Init(const DXContext& dxContext);

	void BeginFrame(const DXContext& dxContext);

	void EndFrame(const DXContext& dxContext);

	void Present();

	void Close();

	void Update();

	void Resize(const DXContext& dxContext);

	uint32_t GetBackBufferCount() const;

	void SetFullScreen(bool enable);

	bool ShouldClose();

	bool ShouldResize();

	bool IsFullScreen() const;

	uint32_t GetWidth() const { return m_width; }
	uint32_t GetHeight() const { return m_height; }

private:
	void SetResolutionToMonitor();
	void CreateWindowHandle();
	void CreateSwapChain(const DXContext& dxContext);

	void GetBuffers(const DXContext& dxContext);

	void ReleaseBuffers();

	ATOM m_wndClassAtom;
	HWND m_handle;

	bool m_shouldClose = false;
	bool m_shouldResize = false;
	bool m_fullScreen = false;

	ComPtr<IDXGISwapChain4> m_swapChain;
	std::vector<ComPtr<ID3D12Resource2>> m_buffers;

	ComPtr<ID3D12DescriptorHeap> m_rtvDescHeap;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtvHandles;

	uint32_t m_width;
	uint32_t m_height;

	uint32_t m_backbufferCount = 3u;
	uint32_t m_currentBufferIndex = 0u;
};
