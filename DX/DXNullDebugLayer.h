#pragma once
#include "IDXDebugLayer.h"

class DXNullDebugLayer : public IDXDebugLayer
{
public:
	virtual ~DXNullDebugLayer() {}
	virtual void Init() override {}

	virtual void PIXCaptureAndOpen() {};
	virtual void RenderdocCaptureStart() {};
	virtual void RenderdocCaptureEnd() {};
};
