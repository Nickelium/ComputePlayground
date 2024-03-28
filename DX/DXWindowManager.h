#pragma once
#include "../core/Common.h"

class DXWindowManager
{
public:
	DXWindowManager();
	~DXWindowManager();

	std::wstring GetWindowClassExName() const;
private:
	void Init();
	void Close();

	ATOM m_wnd_class_atom;
	WNDCLASSEXW m_wnd_class_exw;
	std::wstring m_wnd_class_name;
};