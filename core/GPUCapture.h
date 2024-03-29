#pragma once
#include "Common.h"

class GPUCapture
{
public:
	GPUCapture() {};
	virtual ~GPUCapture() {}

	virtual void StartCapture() = 0;
	virtual void EndCapture() = 0;
	virtual void OpenCapture() = 0;
};

class PIXCapture : public GPUCapture
{
public:
	PIXCapture();
	virtual ~PIXCapture();
	virtual void StartCapture();
	virtual void EndCapture();
	virtual void OpenCapture();
private:
	void Init();
	void Close();

	HMODULE m_pix_module;
	std::string m_pix_absolute_path;
};

struct RENDERDOC_API_1_6_0;

class RenderDocCapture : public GPUCapture
{
public:
	RenderDocCapture();
	virtual ~RenderDocCapture();
	virtual void StartCapture();
	virtual void EndCapture();
	virtual void OpenCapture();
private:
	void Init();
	void Close();

	HMODULE m_renderdoc_module;
	RENDERDOC_API_1_6_0* m_renderdoc_api;
	std::string m_renderdoc_absolute_path;
};