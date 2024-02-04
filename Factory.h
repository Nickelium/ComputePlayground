#pragma once
#include "Common.h"
#include "DX/IDXDebugLayer.h"
#include "DX/DXContext.h"
#include "DX/DXCompiler.h"
#include "DX/DXWindow.h"

std::shared_ptr<IDXDebugLayer> CreateDebugLayer(const GRAPHICS_DEBUGGER_TYPE gd_type);
std::shared_ptr<DXContext> CreateDXContext(const GRAPHICS_DEBUGGER_TYPE gd_type);
std::shared_ptr<DXCompiler> CreateDXCompiler();
std::shared_ptr<DXWindow> CreateDXWindow(const DXContext& dx_context, State* state, const std::string& window_name);