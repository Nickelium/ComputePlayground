#pragma once
#include "../core/Common.h"
#include <dxgiformat.h>

class DXContext;

struct IDXGISwapChain4;
struct ID3D12Resource2;
struct ID3D12DescriptorHeap;
struct D3D12_CPU_DESCRIPTOR_HANDLE;

struct State;

class DXWindowManager
{
public:
	DXWindowManager();
	~DXWindowManager();

	LPCWSTR GetWindoClassExName() const;
private:
	void Init();
	void Close();

	ATOM m_wnd_class_atom;
	WNDCLASSEXW m_wnd_class_exw;
	std::wstring m_wnd_class_name;
};

class DXWindow
{
public:
	static LRESULT CALLBACK OnWindowMessage(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param);

	enum class WindowMode
	{
		Normal = 0,
		Maximize,
		Borderless,
		Count
		// We dont support exclusive fullscreen
	};

	DXWindow(const DXContext& dx_context, const DXWindowManager& window_manager, State* state, const std::string& window_name);
	~DXWindow();
	void Init(const DXContext& dx_context, const DXWindowManager& window_manager, const std::string& window_name);

	void BeginFrame(const DXContext& dx_context);

	void EndFrame(const DXContext& dx_context);

	void Present();

	void Close();

	void Update();

	void Resize(const DXContext& dx_context);

	void ToggleWindowMode();

	uint32 GetBackBufferCount() const;

	bool ShouldClose();

	bool ShouldResize();

	uint32 GetWidth() const { return m_width; }
	uint32 GetHeight() const { return m_height; }

	DXGI_FORMAT GetFormat();
private:
	void ApplyWindowMode();
	void SetResolutionToMonitor();
	void CreateWindowHandle(const DXWindowManager& window_manager, const std::string& window_name);
	void CreateSwapChain(const DXContext& dx_context);

	void GetBuffers(const DXContext& dx_context);

	void ReleaseBuffers();

	HWND m_handle;

	bool m_should_close = false;
	bool m_should_resize = false;

	WindowMode m_window_mode;
	WindowMode m_window_mode_request;

	ComPtr<IDXGISwapChain4> m_swap_chain;
	std::vector<ComPtr<ID3D12Resource2>> m_buffers;

	ComPtr<ID3D12DescriptorHeap> m_rtv_desc_heap;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtv_handles;

	uint32 m_width;
	uint32 m_height;

	uint32 m_current_buffer_index = 0u;

	State* m_state;

	bool m_hdr;
};
