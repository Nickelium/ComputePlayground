#include "DXWindow.h"
#include "DXCommon.h"
#include "DXContext.h"
#include "DXWindowManager.h"
#include "DXResource.h"
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//DXWindow* DXWindowManager::CreateDXWindow(const WindowDesc& window_desc)
//{
//	std::wstring wnd_class_name = std::to_wstring(window_desc.m_window_name);
//	WNDCLASSEXW wnd_class_exw =
//	{
//		.cbSize = sizeof(m_wnd_class_exw),
//		.style = CS_HREDRAW | CS_VREDRAW,
//		.lpfnWndProc = window_desc.m_callback,
//		.cbClsExtra = 0,
//		.cbWndExtra = 0,
//		.hInstance = GetModuleHandle(nullptr),
//		.hIcon = LoadIconW(nullptr, IDI_APPLICATION),
//		.hCursor = LoadCursorW(nullptr, IDC_ARROW),
//		.hbrBackground = nullptr,
//		.lpszMenuName = nullptr,
//		.lpszClassName = wnd_class_name.c_str(),
//		.hIconSm = LoadIconW(nullptr, IDI_APPLICATION),
//	};
//	ATOM wnd_class_atom = RegisterClassExW(&m_wnd_class_exw);
//	ASSERT(wnd_class_atom);
//
//	WindowInfo* window_info = new WindowInfo();
//
//	// Client rect matches with backbuffer size but window rect needs to be slightly bigger for borders
//	// Client rect != Window rect in windowed mode only
//	RECT window_rect{ window_info->m_non_fullscreen_area };
//	// Pass in client rect and then make it into window rect
//	DWORD style{ window_info->m_style };
//	style |= WS_OVERLAPPEDWINDOW;
//	bool result = AdjustWindowRect(&window_rect, style, false);
//	ASSERT(result);
//
//	HWND handle = CreateWindow
//	(
//		wnd_class_name.c_str(),
//		std::to_wstring(window_desc.m_window_name).c_str(),
//		style,
//		window_desc.m_origin_x,
//		window_desc.m_origin_y,
//		window_desc.m_width,
//		window_desc.m_height,
//		nullptr, nullptr,
//		GetModuleHandleW(nullptr), this
//	);
//	ASSERT(handle);
//	ASSERT(IsWindow(handle));
//
//	window_info->m_wnd_class_atom = wnd_class_atom;
//	window_info->m_handle = handle;
//
//	return nullptr;
//}

// Global across all windows
LRESULT DXWindow::OnWindowMessage(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// TODO pass callback instead because each window might do different things

	// Get DXWindow Object
	DXWindow* pWindow{};
	// First message send
	if (msg == WM_CREATE)
	{
		// Store DXWindow Object
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		pWindow = reinterpret_cast<DXWindow*>(pCreate->lpCreateParams);
		SetWindowLongPtr(handle, GWLP_USERDATA, (LONG_PTR)pWindow);
	}
	else
	{
		// Retrieve DXWindow Object
		const LONG_PTR ptr = GetWindowLongPtr(handle, GWLP_USERDATA);
		pWindow = reinterpret_cast<DXWindow*>(ptr);
	}

	switch (msg)
	{
	case WM_SIZE:
	{
		uint32 reqWidth = LOWORD(lParam);
		uint32 reqHeight = HIWORD(lParam);
		if
		(
			lParam &&
			(reqWidth != pWindow->m_width || reqHeight != pWindow->m_height)
		)
		{
			pWindow->m_should_resize = true;
		}

		// Switch to maximize state
		if (wParam & SIZE_MAXIMIZED)
		{
			pWindow->m_window_mode_request = WindowMode::Maximize;
		}
	}
	break;
	case WM_KEYDOWN:
		pWindow->input.SetKeyPressed(static_cast<int32>(wParam), true);
		return 0;
	case WM_KEYUP:
		pWindow->input.SetKeyPressed(static_cast<int32>(wParam), false);
		return 0;
	case WM_CLOSE:
		pWindow->m_should_close = true;
		return 0;
	default:
		break;
	}
	return DefWindowProcW(handle, msg, wParam, lParam);
}

// Definition
uint32 g_current_buffer_index = 0u;

DXWindow::DXWindow(const DXContext& dx_context, const DXWindowManager& window_manager, const WindowDesc& window_desc)
{
	Init(dx_context, window_manager, window_desc);
}

DXWindow::~DXWindow()
{
	Close();
}

void DXWindow::Init
 (
	 const DXContext& dx_context, const DXWindowManager& window_manager, 
	 const WindowDesc& window_desc
 )
{
	m_hdr = false;

	m_window_mode = WindowMode::Normal;
	m_window_mode_request = m_window_mode;

	m_width = m_windowed_width = window_desc.m_width;
	m_height = m_windowed_height =  window_desc.m_height;
	m_windowed_origin_x = window_desc.m_origin_x;
	m_windowed_origin_y = window_desc.m_origin_y;

	CreateWindowHandle(window_manager, window_desc.m_window_name);
	ApplyWindowStyle();
	ApplyWindowMode();
	CreateSwapChain(dx_context);

	Microsoft::WRL::ComPtr<IDXGIFactory1> factory{};
	m_swap_chain->GetParent(IID_PPV_ARGS(&factory)) >> CHK;
	// Requires to GetParent factory to work

	// Disable ALT ENTER to disable exclusive fullscreen, we handle switching ourselves
	factory->MakeWindowAssociation(m_handle, DXGI_MWA_NO_ALT_ENTER) >> CHK;

	dx_context.CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, GetBackBufferCount(), "Descriptor Heap RTVs", m_rtv_desc_heap);

	m_rtv_handles.resize(GetBackBufferCount());
	for (uint32 i = 0; i < m_rtv_handles.size(); ++i)
	{
		m_rtv_handles[i] = dx_context.GetDescriptorHandle(m_rtv_desc_heap, i);
	}
	
	GetBuffers();

	for (uint32 i = 0; i < m_buffers.size(); ++i)
	{
		dx_context.CreateRTV(m_buffers[i], GetFormat(), m_rtv_handles[i]);
	}
}

void DXWindow::BeginFrame(const DXContext& dx_context)
{
	dx_context.Transition(D3D12_RESOURCE_STATE_RENDER_TARGET, m_buffers[g_current_buffer_index]);

	const float4 color{ 85.0f / 255.0f, 230.0f / 255.0f, 23.0f / 255.0f, 1.0f };
	// TODO replace with rtv descriptor manager
	dx_context.GetCommandListGraphics()->ClearRenderTargetView(m_rtv_handles[g_current_buffer_index], color, 0, nullptr);
	dx_context.GetCommandListGraphics()->OMSetRenderTargets(1, &m_rtv_handles[g_current_buffer_index], false, nullptr);
}

void DXWindow::EndFrame(const DXContext& dx_context)
{
	dx_context.Transition(D3D12_RESOURCE_STATE_PRESENT, m_buffers[g_current_buffer_index]);
}

void DXWindow::Present(DXContext& dx_context)
{
	// Sync interval [0;4] where 0 is uncapped
	uint32 vsync_interval = 0;
	uint32 present_flags = 0;
	m_swap_chain->Present(vsync_interval, present_flags) >> CHK;
	dx_context.Signal(dx_context.m_queue_graphics, dx_context.m_fence, g_current_buffer_index);

	g_current_buffer_index = m_swap_chain->GetCurrentBackBufferIndex();
}

void DXWindow::Close()
{
	if (m_handle)
	{
		ASSERT(DestroyWindow(m_handle));
	}
}

void DXWindow::Update()
{
	MSG msg{};
	while (PeekMessageW(&msg, m_handle, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	if (m_window_mode_request != m_window_mode)
	{
		m_window_mode = m_window_mode_request;
		ApplyWindowStyle();
		ApplyWindowMode();
	}

	// Store windowed size after apply, to restore later on
	if (m_window_mode_request == WindowMode::Normal)
	{
		m_windowed_width = m_width;
		m_windowed_height = m_height;
		RECT rect{};
		bool result = GetWindowRect(m_handle, &rect);
		ASSERT(result);
		m_windowed_origin_x = rect.left;
		m_windowed_origin_y = rect.top;
	}
}

void DXWindow::Resize(const DXContext& dx_context)
{
	RECT rt{};
	if (GetClientRect(m_handle, &rt))
	{
		ReleaseBuffers();

		m_width = rt.right - rt.left;
		m_height = rt.bottom - rt.top;
		DXGI_SWAP_CHAIN_DESC1 desc{};
		m_swap_chain->GetDesc1(&desc);
		m_swap_chain->ResizeBuffers(GetBackBufferCount(), GetWidth(), GetHeight(), desc.Format, desc.Flags) >> CHK;
		m_should_resize = false;

		GetBuffers();
		// Recreate RTVs
		// Dont need to recreate descriptors nor descriptor heap
		for (uint32 i = 0; i < m_buffers.size(); ++i)
		{
			dx_context.CreateRTV(m_buffers[i], GetFormat(), m_rtv_handles[i]);
		}
	}
}

void DXWindow::SetWindowModeRequest(WindowMode window_mode)
{
	m_window_mode_request = window_mode;
}

WindowMode DXWindow::GetWindowModeRequest() const
{
	return m_window_mode_request;
}

uint32 DXWindow::GetBackBufferCount() const
{
	return g_backbuffer_count;
}

void DXWindow::ApplyWindowStyle()
{
	// Set styles
	if (m_window_mode == WindowMode::Normal)
	{
		m_style = WS_OVERLAPPEDWINDOW;
	}
	else if (m_window_mode == WindowMode::Maximize)
	{
		m_style = WS_OVERLAPPEDWINDOW;
	}
	else
	{
		ASSERT(m_window_mode == WindowMode::Borderless);
		m_style = WS_POPUP;
	}
	SetWindowLongW(m_handle, GWL_STYLE, m_style);
}

// There should be 3 modes, normal window, maximized window and borderless window
// Normal window : WS_OVERLAPPED, shown with ShowWindow(hwnd, SW_SHOW)
// Maximized window : WS_OVERLAPPED, shown with ShowWindow(hwnd, SW_MAXMIZE) covers the whole screen, not including the taskbar
// Borderless window : WS_POPUP flag, with width& height set to SM_CXSCREEN / SM_CYSCREEN, covers the whole screen, it goes over the task bar
void DXWindow::ApplyWindowMode()
{
	// Set show window
	if (m_window_mode == WindowMode::Normal)
	{
		// Client rect matches with backbuffer size but window rect needs to be slightly bigger for borders
		// Client rect != Window rect in windowed mode only
		RECT window_rect{ 0, 0, static_cast<LONG>(m_windowed_width), static_cast<LONG>(m_windowed_height) };
		// Pass in client rect and then make it into window rect
		bool result = AdjustWindowRect(&window_rect, m_style, false);
		ASSERT(result);
		// Restore previous setting
		SetWindowPos
		(
			m_handle, nullptr,
			m_windowed_origin_x, m_windowed_origin_y,
			window_rect.right - window_rect.left, window_rect.bottom - window_rect.top,
			SWP_NOZORDER
		);
		ShowWindow(m_handle, SW_SHOW);
	}
	else if (m_window_mode == WindowMode::Maximize)
	{
		ShowWindow(m_handle, SW_MAXIMIZE);
	}
	else
	{
		ASSERT(m_window_mode == WindowMode::Borderless);
		const HMONITOR monitorHandle = MonitorFromWindow(m_handle, MONITOR_DEFAULTTONEAREST);
		MONITORINFO monitorInfo{};
		monitorInfo.cbSize = sizeof(monitorInfo);
		if (GetMonitorInfoW(monitorHandle, &monitorInfo))
		{
			SetWindowPos(m_handle, nullptr,
			             monitorInfo.rcMonitor.left,
			             monitorInfo.rcMonitor.top,
			             monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
			             monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top, SWP_NOZORDER);
		}
		// SetWindowLongW(!WS_VISIBLE) to avoid semi black / white frame
		// No ShowWindow for borderless because it breaks
		// But then needs an additional SetWindowLongW with WS_VISIBLE to work with sequence SetWindowLongW(!WS_VISIBLE) -> NoShowWindow
		SetWindowLongW(m_handle, GWL_STYLE, m_style | WS_VISIBLE);
	}
}

bool DXWindow::ShouldClose() const
{
	return m_should_close;
}

bool DXWindow::ShouldResize()
{
	return m_should_resize;
}

static const DXGI_FORMAT dxgi_format_sdr = DXGI_FORMAT_R8G8B8A8_UNORM;
static const DXGI_FORMAT dxgi_format_hdr = DXGI_FORMAT_R10G10B10A2_UNORM;

DXGI_FORMAT GetBackBufferFormat(bool hdr)
{
	return hdr ? dxgi_format_hdr : dxgi_format_sdr;
}

WindowInfo CreateWindowNew(const WindowDesc& window_desc)
{
	std::wstring wnd_class_name = std::to_wstring(window_desc.m_window_name);
	WNDCLASSEX wnd_class_exw =
	{
		.cbSize = sizeof(WNDCLASSEX),
		.style = CS_HREDRAW | CS_VREDRAW,
		// Note callback must handle WM_CREATE eg. through DefWinProc
		.lpfnWndProc = window_desc.m_callback,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = GetModuleHandle(nullptr),
		.hIcon = LoadIconW(nullptr, IDI_APPLICATION),
		.hCursor = LoadCursorW(nullptr, IDC_ARROW),
		.hbrBackground = nullptr,
		.lpszMenuName = nullptr,
		.lpszClassName = wnd_class_name.c_str(),
		.hIconSm = LoadIconW(nullptr, IDI_APPLICATION),
	};
	ATOM wnd_class_atom = RegisterClassEx(&wnd_class_exw);
	ASSERT(wnd_class_atom);

	WindowInfo window_info{};

	RECT window_rect{ window_info.m_client_area };
	window_info.m_style |= WS_OVERLAPPEDWINDOW;
	bool result = AdjustWindowRect(&window_rect, window_info.m_style, false);
	ASSERT(result);
	auto er = GetLastError();
	window_info.m_handle = CreateWindowExW
	(
		0,
		wnd_class_name.c_str(),
		wnd_class_name.c_str(),
		window_info.m_style,
		window_desc.m_origin_x,
		window_desc.m_origin_y,
		window_desc.m_width,
		window_desc.m_height,
		nullptr, nullptr,
		GetModuleHandleW(nullptr), nullptr
	);
	er = GetLastError();
	ASSERT(window_info.m_handle);
	ASSERT(IsWindow(window_info.m_handle));

	//window_info->m_wnd_class_atom = wnd_class_atom;
	// TODO
	return window_info;
}

DXGI_FORMAT DXWindow::GetFormat() const
{
	return GetBackBufferFormat(m_hdr);
}

void DXWindow::CreateWindowHandle(const DXWindowManager& window_manager, const std::string& window_name)
{
	// Client rect matches with backbuffer size but window rect needs to be slightly bigger for borders
	// Client rect != Window rect in windowed mode only
	RECT window_rect{ 0, 0, static_cast<LONG>(m_windowed_width), static_cast<LONG>(m_windowed_height) };
	// Pass in client rect and then make it into window rect
	bool result = AdjustWindowRect(&window_rect, m_style, false);
	ASSERT(result);

	m_handle = CreateWindow
	(
		window_manager.GetWindowClassExName().c_str(),
		std::to_wstring(window_name).c_str(),
		m_style, 
		m_windowed_origin_x,
		m_windowed_origin_y,
		window_rect.right - window_rect.left,
		window_rect.bottom - window_rect.top,
		nullptr, nullptr,
		GetModuleHandleW(nullptr), this
	);
	ASSERT(m_handle);
	ASSERT(IsWindow(m_handle));
}

void DXWindow::CreateSwapChain(const DXContext& dxContext)
{
	const DXGI_SWAP_CHAIN_DESC1 swapChainDesc =
	{
		.Width = GetWidth(),
		.Height = GetHeight(),
		.Format = GetFormat(),
		.Stereo = false, // No VR rendering
		.SampleDesc =
		{
			.Count = 1,
			.Quality = 0,
		},
		.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = g_backbuffer_count,
		.Scaling = DXGI_SCALING_NONE,
		// FLIP_DISCARD (can discard backbuffer after present) or FLIP_SEQ (keeps backbuffer alive after present) in DX12
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.AlphaMode = DXGI_ALPHA_MODE_IGNORE,
		.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING,
	};
	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain{};
	Microsoft::WRL::ComPtr<IDXGIFactory5> factory3{};
	dxContext.GetFactory().As(&factory3) >> CHK;
	factory3->CreateSwapChainForHwnd
	(
		dxContext.GetCommandQueue().m_queue.Get(), m_handle,
		&swapChainDesc, nullptr, nullptr,
		&swapChain
	) >> CHK;
	swapChain.As(&m_swap_chain) >> CHK;
	NAME_DXGI_OBJECT(m_swap_chain, "SwapChain");
}

void DXWindow::GetBuffers()
{
	// Get backbuffer resources
	m_buffers.resize(GetBackBufferCount());
	for (uint32 i = 0; i < GetBackBufferCount(); ++i)
	{
		m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&m_buffers[i].m_resource)) >> CHK;
		std::string str = "Backbuffer " + std::to_string(i);
		NAME_DX_OBJECT(m_buffers[i].m_resource, str.c_str());
	}
}

void DXWindow::ReleaseBuffers()
{
	for (uint32 i = 0; i < GetBackBufferCount(); ++i)
	{
		m_buffers[i].m_resource = nullptr;
	}
}

