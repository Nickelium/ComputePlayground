#pragma once
#include "IDXDebugLayer.h"

class DXNullDebugLayer : public IDXDebugLayer
{
public:
	virtual ~DXNullDebugLayer() {}
	virtual void Init() override {}
	virtual void Close() override {}
};
