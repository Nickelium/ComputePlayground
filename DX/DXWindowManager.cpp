#include "DXWindowManager.h"
#include "DXWindow.h"

DXWindowManager::DXWindowManager()
{
	Init();
}

DXWindowManager::~DXWindowManager()
{
	Close();
}

std::wstring DXWindowManager::GetWindowClassExName() const
{
	return m_wnd_class_name;
}

void DXWindowManager::Init()
{
	// Ignore windows resolution scale
	SetProcessDPIAware();

	m_wnd_class_name = { L"WndClass" };
	m_wnd_class_exw =
	{
		.cbSize = sizeof(m_wnd_class_exw),
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = DXWindow::OnWindowMessage,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = GetModuleHandle(nullptr),
		.hIcon = LoadIconW(nullptr, IDI_APPLICATION),
		.hCursor = LoadCursorW(nullptr, IDC_ARROW),
		.hbrBackground = nullptr,
		.lpszMenuName = nullptr,
		.lpszClassName = m_wnd_class_name.c_str(),
		.hIconSm = LoadIconW(nullptr, IDI_APPLICATION),
	};
	m_wnd_class_atom = RegisterClassExW(&m_wnd_class_exw);
	ASSERT(m_wnd_class_atom);
}

void DXWindowManager::Close()
{
	if (m_wnd_class_atom)
	{
		ASSERT(UnregisterClassW((LPCWSTR)m_wnd_class_atom, GetModuleHandleW(nullptr)));
	}
}