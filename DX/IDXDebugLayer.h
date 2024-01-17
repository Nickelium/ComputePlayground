#pragma once

class IDXDebugLayer
{
public:
	virtual ~IDXDebugLayer() { }
	virtual void Init() = 0;
};
