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
	static LRESULT CALLBACK OnWindowMessage(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param);

	void Init(const DXContext& dx_context);

	void BeginFrame(const DXContext& dx_context);

	void EndFrame(const DXContext& dx_context);

	void Present();

	void Close();

	void Update();

	void Resize(const DXContext& dx_context);

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
	void CreateSwapChain(const DXContext& dx_context);

	void GetBuffers(const DXContext& dx_context);

	void ReleaseBuffers();

	ATOM m_wnd_class_atom;
	HWND m_handle;

	bool m_should_close = false;
	bool m_should_resize = false;
	bool m_full_screen = false;

	ComPtr<IDXGISwapChain4> m_swap_chain;
	std::vector<ComPtr<ID3D12Resource2>> m_buffers;

	ComPtr<ID3D12DescriptorHeap> m_rtv_desc_heap;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtv_handles;

	uint32_t m_width;
	uint32_t m_height;

	uint32_t m_backbuffer_count = 3u;
	uint32_t m_current_buffer_index = 0u;
};
