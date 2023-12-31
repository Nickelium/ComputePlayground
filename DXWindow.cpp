#include "DXWindow.h"
#include "DXCommon.h"
#include "DXDebugLayer.h"
#include "DXContext.h"

LRESULT CALLBACK DXWindow::OnWindowMessage(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Get DXWindow Object
	DXWindow* pWindow = nullptr;
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
		LONG_PTR ptr = GetWindowLongPtr(handle, GWLP_USERDATA);
		pWindow = reinterpret_cast<DXWindow*>(ptr);
	}

	switch (msg)
	{
	case WM_SIZE:
	{
		// Handle minimize & maximize
		uint32_t reqWidth = HIWORD(lParam);
		uint32_t reqHeight = LOWORD(lParam);
		if
			(
				lParam &&
				(reqWidth != pWindow->m_width || reqHeight != pWindow->m_height)
				)
		{
			pWindow->m_should_resize = true;
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
			pWindow->SetFullScreen(!pWindow->IsFullScreen());
		}
		else if (wParam == VK_F1)
		{
			PIXCaptureAndOpen();
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

void DXWindow::Init(const DXContext& dxContext)
{
	SetResolutionToMonitor();
	CreateWindowHandle();
	CreateSwapChain(dxContext);

	ComPtr<IDXGIFactory1> factory;
	m_swap_chain->GetParent(IID_PPV_ARGS(&factory)) >> CHK;
	// Requires to GetParent factory to work
	factory->MakeWindowAssociation(m_handle, DXGI_MWA_NO_ALT_ENTER) >> CHK;

	D3D12_DESCRIPTOR_HEAP_DESC rtvDescHeapDesc =
	{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = GetBackBufferCount(),
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0,
	};
	dxContext.GetDevice()->CreateDescriptorHeap(&rtvDescHeapDesc, IID_PPV_ARGS(&m_rtv_desc_heap)) >> CHK;

	m_rtv_handles.resize(GetBackBufferCount());
	D3D12_CPU_DESCRIPTOR_HANDLE firstHandle = m_rtv_desc_heap->GetCPUDescriptorHandleForHeapStart();
	uint32_t incrementSize = dxContext.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (uint32_t i = 0; i < GetBackBufferCount(); ++i)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE currentHandle{};
		currentHandle.ptr = firstHandle.ptr + i * incrementSize;
		m_rtv_handles[i] = currentHandle;
	}

	GetBuffers(dxContext);
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
	dxContext.GetCommandList()->ResourceBarrier(_countof(barrier), barrier);

	float color[4]{ 85.0f / 255.0f, 230.0f / 255.0f, 23.0f / 255.0f, 1.0f };
	dxContext.GetCommandList()->ClearRenderTargetView(m_rtv_handles[m_current_buffer_index], color, 0, nullptr);
	dxContext.GetCommandList()->OMSetRenderTargets(1, &m_rtv_handles[m_current_buffer_index], false, nullptr);
}

void DXWindow::EndFrame(const DXContext& dxContext)
{
	D3D12_RESOURCE_BARRIER barrier[1];
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
	dxContext.GetCommandList()->ResourceBarrier(_countof(barrier), barrier);
}

void DXWindow::Present()
{
	m_swap_chain->Present(1, 0) >> CHK;
}

void DXWindow::Close()
{
	if (m_handle)
	{
		ASSERT(DestroyWindow(m_handle));
	}

	if (m_wnd_class_atom)
	{
		ASSERT(UnregisterClassW((LPCWSTR)m_wnd_class_atom, GetModuleHandleW(nullptr)));
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
		m_swap_chain->ResizeBuffers(GetBackBufferCount(), m_width, m_height, desc.Format, desc.Flags) >> CHK;
		m_should_resize = false;

		GetBuffers(dxContext);
	}
}

uint32_t DXWindow::GetBackBufferCount() const
{
	return m_backbuffer_count;
}

void DXWindow::SetFullScreen(bool enable)
{
	DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	DWORD exStyle = WS_EX_OVERLAPPEDWINDOW | WS_EX_APPWINDOW;
	if (enable)
	{
		style = WS_POPUP | WS_VISIBLE;
		exStyle = WS_EX_APPWINDOW;
	}
	SetWindowLongW(m_handle, GWL_STYLE, style);
	SetWindowLongW(m_handle, GWL_EXSTYLE, exStyle);

	if (enable)
	{
		HMONITOR monitorHandle = MonitorFromWindow(m_handle, MONITOR_DEFAULTTONEAREST);
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
	}
	else
	{
		ShowWindow(m_handle, SW_MAXIMIZE);
	}

	m_full_screen = enable;
}

bool DXWindow::ShouldClose()
{
	return m_should_close;
}

bool DXWindow::ShouldResize()
{
	return m_should_resize;
}

bool DXWindow::IsFullScreen() const
{
	return m_full_screen;
}

void DXWindow::SetResolutionToMonitor()
{
	POINT pt{};
	HMONITOR monitorHandle = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
	MONITORINFO monitorInfo{};
	monitorInfo.cbSize = sizeof(monitorInfo);
	bool success = GetMonitorInfoW(monitorHandle, &monitorInfo);
	/*assert*/(success);
	m_width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
		m_height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
}

void DXWindow::CreateWindowHandle()
{
	WNDCLASSEXW wndClassExW =
	{
		.cbSize = sizeof(wndClassExW),
		.style = CS_OWNDC,
		.lpfnWndProc = DXWindow::OnWindowMessage,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = GetModuleHandle(nullptr),
		.hIcon = LoadIconW(nullptr, IDI_APPLICATION),
		.hCursor = LoadCursorW(nullptr, IDC_ARROW),
		.hbrBackground = nullptr,
		.lpszMenuName = nullptr,
		// TODO unique identifier to support multi windows
		.lpszClassName = L"WndClass",
		.hIconSm = LoadIconW(nullptr, IDI_APPLICATION),
	};
	m_wnd_class_atom = RegisterClassExW(&wndClassExW);
	ASSERT(m_wnd_class_atom);

	m_handle = CreateWindowExW
	(
		WS_EX_OVERLAPPEDWINDOW | WS_EX_APPWINDOW, (LPCWSTR)m_wnd_class_atom,
		L"Application", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, m_width, m_height,
		nullptr, nullptr,
		GetModuleHandleW(nullptr), this
	);
	ASSERT(m_handle);
	ASSERT(IsWindow(m_handle));
}

void DXWindow::CreateSwapChain(const DXContext& dxContext)
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc =
	{
		.Width = m_width,
		.Height = m_height,
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.Stereo = false,
		.SampleDesc =
		{
			.Count = 1,
			.Quality = 0,
		},
		.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = m_backbuffer_count,
		.Scaling = DXGI_SCALING_NONE,
		// FLIP_DISCARD (can discard backbuffer after present) or FLIP_SEQ (keeps backbuffer alive after present) in DX12
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.AlphaMode = DXGI_ALPHA_MODE_IGNORE,
		.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING,
	};
	ComPtr<IDXGISwapChain1> swapChain;
	dxContext.GetFactory()->CreateSwapChainForHwnd
	(
		dxContext.GetCommandQueue().Get(), m_handle,
		&swapChainDesc, nullptr, nullptr,
		&swapChain
	) >> CHK;
	swapChain->QueryInterface(IID_PPV_ARGS(&m_swap_chain)) >> CHK;
	m_swap_chain->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(wchar_t) * _countof(L"SwapChain"), L"SwapChain") >> CHK;
}

void DXWindow::GetBuffers(const DXContext& dxContext)
{
	m_buffers.resize(GetBackBufferCount());

	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescStart = m_rtv_desc_heap->GetCPUDescriptorHandleForHeapStart();
	for (uint32_t i = 0; i < GetBackBufferCount(); ++i)
	{
		m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&m_buffers[i])) >> CHK;
		std::wstring str = L"Backbuffer " + std::to_wstring(i);
		m_buffers[i]->SetName(str.c_str());

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc =
		{
			// SDR Format
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
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
	for (uint32_t i = 0; i < GetBackBufferCount(); ++i)
	{
		m_buffers[i] = nullptr;
	}
}

