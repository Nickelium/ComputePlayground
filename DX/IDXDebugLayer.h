#pragma once

class IDXDebugLayer
{
public:
	virtual ~IDXDebugLayer() { }
	virtual void Init() = 0;

	virtual void PIXCaptureAndOpen() = 0;
	virtual void RenderdocCaptureStart() = 0;
	virtual void RenderdocCaptureEnd() = 0;
};
