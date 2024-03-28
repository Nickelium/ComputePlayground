#pragma once
#include "../core/Common.h"

class DXContext;

struct IDXGISwapChain4;
struct ID3D12Resource2;
struct ID3D12DescriptorHeap;
struct D3D12_CPU_DESCRIPTOR_HANDLE;

struct State;

class DXWindow
{
public:
	static LRESULT CALLBACK OnWindowMessage(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param);
	static void GlobalInit();
	static void GlobalClose();

	enum class WindowMode
	{
		Normal = 0,
		Maximize,
		Borderless,
		Count
		// We dont support exclusive fullscreen
	};

	DXWindow(const DXContext& dx_context, State* state, const std::string& window_name);
	~DXWindow();
	void Init(const DXContext& dx_context, const std::string& window_name);

	void BeginFrame(const DXContext& dx_context);

	void EndFrame(const DXContext& dx_context);

	void Present();

	void Close();

	void Update();

	void Resize(const DXContext& dx_context);

	void ToggleWindowMode();

	uint32_t GetBackBufferCount() const;

	bool ShouldClose();

	bool ShouldResize();

	uint32_t GetWidth() const { return m_width; }
	uint32_t GetHeight() const { return m_height; }

private:
	// Declaration of static class variables
	static ATOM s_wnd_class_atom;
	static WNDCLASSEXW s_wnd_class_exw;
	static std::wstring s_wnd_class_name;
	static bool s_is_initialized;

	void ApplyWindowMode();
	void SetResolutionToMonitor();
	void CreateWindowHandle(const std::string& window_name);
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

	uint32_t m_width;
	uint32_t m_height;

	uint32_t m_current_buffer_index = 0u;

	State* m_state;
};
