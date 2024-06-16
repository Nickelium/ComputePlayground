#pragma once
#include "../core/Common.h"
#include <dxgiformat.h>
#include "DXContext.h"

class DXContext;
class DXWindowManager;

struct IDXGISwapChain4;
struct ID3D12Resource2;
struct DescriptorHeap;
struct D3D12_CPU_DESCRIPTOR_HANDLE;

struct State;


////////////
// NEW WINDOW SECTION

/// 

using WindowProc = LRESULT(*)(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam);
struct WindowDesc
{
	WindowProc m_callback;
	std::string m_window_name;
	uint32 m_width;
	uint32 m_height;
	uint32 m_origin_x;
	uint32 m_origin_y;
};

struct WindowInfo
{
	HWND m_handle{0};
	RECT m_client_area{0, 0, 1000, 1000};
	RECT m_fullscreen_area;
	POINT m_top_left{0, 0};
	DWORD m_style{WS_VISIBLE};
	bool m_is_fullscreen{ false };
	bool m_is_closed{ false };

	//ATOM m_wnd_class_atom;
	//WNDCLASSEXW m_wnd_class_exw;
	//std::wstring m_wnd_class_name;
};

WindowInfo CreateWindowNew(const WindowDesc& window_desc);

// We dont support exclusive fullscreen
enum class WindowMode
{
	Normal = 0,
	Maximize,
	Borderless,
	Count
};

enum D3D12_RESOURCE_STATES;
class DXResource;

class Input 
{
public:
	bool IsKeyPressed(int32 key)
	{
		ASSERT(key < s_number_of_keys);
		return m_is_key_pressed[key];
	}

	void SetKeyPressed(int32 key, bool is_pressed)
	{
		ASSERT(key < s_number_of_keys);
		m_is_key_pressed[key] = is_pressed;
	}
private:
	static const int32 s_number_of_keys = 255;
	bool m_is_key_pressed[s_number_of_keys];

};

class DXWindow
{
public:
	static LRESULT CALLBACK OnWindowMessage(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param);

	DXWindow(const DXContext& dx_context, const DXWindowManager& window_manager, const WindowDesc& window_desc);
	~DXWindow();
	void Init
	(
		const DXContext& dx_context, const DXWindowManager& window_manager, 
		const WindowDesc& window_desc
	);

	void BeginFrame(const DXContext& dx_context);

	void EndFrame(const DXContext& dx_context);

	void Present(DXContext& dx_context);

	void Close();

	void Update();

	void Resize(const DXContext& dx_context);

	void SetWindowModeRequest(WindowMode window_mode);
	WindowMode GetWindowModeRequest() const;

	uint32 GetBackBufferCount() const;

	bool ShouldClose() const;

	bool ShouldResize();

	uint32 GetWidth() const { return m_width; }
	uint32 GetHeight() const { return m_height; }

	DXGI_FORMAT GetFormat() const;

	void SetTitle();
	std::string GetTitle();
	bool IsClosed();


	// TODO separate input from window class
	Input input;
	// TODO dont make this public
	bool m_should_close = false;

private:
	void ApplyWindowStyle();
	void ApplyWindowMode();
	void CreateWindowHandle(const DXWindowManager& window_manager, const std::string& window_name);
	void CreateSwapChain(const DXContext& dx_context);

	void GetBuffers();

	void ReleaseBuffers();

	HWND m_handle;

	bool m_should_resize = false;

	WindowMode m_window_mode;
	WindowMode m_window_mode_request;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swap_chain;
public:
	std::vector<DXResource> m_buffers;
private:
	DescriptorHeap m_rtv_desc_heap;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtv_handles;

	uint32 m_width;
	uint32 m_height;
	uint32 m_windowed_origin_x;
	uint32 m_windowed_origin_y;
	uint32 m_windowed_width;
	uint32 m_windowed_height;
	uint32 m_style;


	bool m_hdr;
};

// Declaration
extern uint32 g_current_buffer_index;
