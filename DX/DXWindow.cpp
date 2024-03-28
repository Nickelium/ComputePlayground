#include "DXWindow.h"
#include "DXCommon.h"
#include "DXContext.h"
#include "DXWindowManager.h"

LRESULT CALLBACK DXWindow::OnWindowMessage(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
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
		if (wParam == VK_ESCAPE)
		{
			pWindow->m_should_close = true;
		}
		else if (wParam == VK_F11)
		{
			// F11 also sends a WM_SIZE after
			pWindow->ToggleWindowMode();
		}
		else if (wParam == VK_F1)
		{
			pWindow->m_state->m_capture = true;
		}
		return 0;
	case WM_CLOSE:
		pWindow->m_should_close = true;
		return 0;
	default:
		break;
	}
	return DefWindowProcW(handle, msg, wParam, lParam);
}


DXWindow::DXWindow(const DXContext& dx_context, const DXWindowManager& window_manager, State* state, const WindowDesc& window_desc)
	:m_state(state)
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

	ComPtr<IDXGIFactory1> factory{};
	m_swap_chain->GetParent(IID_PPV_ARGS(&factory)) >> CHK;
	// Requires to GetParent factory to work

	// Disable ALT ENTER to disable exclusive fullscreen
	factory->MakeWindowAssociation(m_handle, DXGI_MWA_NO_ALT_ENTER) >> CHK;

	const D3D12_DESCRIPTOR_HEAP_DESC rtvDescHeapDesc =
	{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = GetBackBufferCount(),
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0,
	};
	dx_context.GetDevice()->CreateDescriptorHeap(&rtvDescHeapDesc, IID_PPV_ARGS(&m_rtv_desc_heap)) >> CHK;

	m_rtv_handles.resize(GetBackBufferCount());
	const D3D12_CPU_DESCRIPTOR_HANDLE firstHandle = m_rtv_desc_heap->GetCPUDescriptorHandleForHeapStart();
	const uint32 incrementSize = dx_context.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (uint32 i = 0; i < GetBackBufferCount(); ++i)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE currentHandle{};
		currentHandle.ptr = firstHandle.ptr + i * incrementSize;
		m_rtv_handles[i] = currentHandle;
	}

	GetBuffers(dx_context);
}

void DXWindow::BeginFrame(const DXContext& dxContext)
{
	m_current_buffer_index = m_swap_chain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER barrier[1]{};
	barrier[0] =
	{
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		.Transition =
		{
			.pResource = m_buffers[m_current_buffer_index].Get(),
			.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
			.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
		}
	};
	dxContext.GetCommandListGraphics()->ResourceBarrier(COUNT(barrier), barrier);

	const float4 color{ 85.0f / 255.0f, 230.0f / 255.0f, 23.0f / 255.0f, 1.0f };
	dxContext.GetCommandListGraphics()->ClearRenderTargetView(m_rtv_handles[m_current_buffer_index], color, 0, nullptr);
	dxContext.GetCommandListGraphics()->OMSetRenderTargets(1, &m_rtv_handles[m_current_buffer_index], false, nullptr);
}

void DXWindow::EndFrame(const DXContext& dxContext)
{
	D3D12_RESOURCE_BARRIER barrier[1]{};
	barrier[0] =
	{
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		.Transition =
		{
			.pResource = m_buffers[m_current_buffer_index].Get(),
			.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
			.StateAfter = D3D12_RESOURCE_STATE_PRESENT,
		}
	};
	dxContext.GetCommandListGraphics()->ResourceBarrier(COUNT(barrier), barrier);
}

void DXWindow::Present()
{
	// Sync interval [0;4] where 0 is uncapped
	uint32 vsync_interval = 0;
	uint32 present_flags = 0;
	m_swap_chain->Present(vsync_interval, present_flags) >> CHK;
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

void DXWindow::Resize(const DXContext& dxContext)
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

		GetBuffers(dxContext);
	}
}

void DXWindow::ToggleWindowMode()
{
	m_window_mode_request = static_cast<WindowMode>((static_cast<int32>(m_window_mode_request) + 1) % (static_cast<int32>(WindowMode::Count)));
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
		.Stereo = false,
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
		.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING,
	};
	ComPtr<IDXGISwapChain1> swapChain{};
	ComPtr<IDXGIFactory5> factory3{};
	dxContext.GetFactory()->QueryInterface(IID_PPV_ARGS(&factory3)) >> CHK;
	factory3->CreateSwapChainForHwnd
	(
		dxContext.GetCommandQueue().Get(), m_handle,
		&swapChainDesc, nullptr, nullptr,
		&swapChain
	) >> CHK;
	swapChain->QueryInterface(IID_PPV_ARGS(&m_swap_chain)) >> CHK;
	NAME_DXGI_OBJECT(m_swap_chain, "SwapChain");
}

void DXWindow::GetBuffers(const DXContext& dxContext)
{
	m_buffers.resize(GetBackBufferCount());

	const D3D12_CPU_DESCRIPTOR_HANDLE rtvDescStart = m_rtv_desc_heap->GetCPUDescriptorHandleForHeapStart();
	for (uint32 i = 0; i < GetBackBufferCount(); ++i)
	{
		m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&m_buffers[i])) >> CHK;
		std::string str = "Backbuffer " + std::to_string(i);
		NAME_DX_OBJECT(m_buffers[i], str.c_str());

		const D3D12_RENDER_TARGET_VIEW_DESC rtvDesc =
		{
			// SDR Format
			.Format = GetFormat(),
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
			.Texture2D =
			{
				.MipSlice = 0,
				.PlaneSlice = 0,
			},
		};
		dxContext.GetDevice()->CreateRenderTargetView(m_buffers[i].Get(), &rtvDesc, m_rtv_handles[i]);
	}
}

void DXWindow::ReleaseBuffers()
{
	for (uint32 i = 0; i < GetBackBufferCount(); ++i)
	{
		m_buffers[i] = nullptr;
	}
}

