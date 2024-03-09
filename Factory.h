#pragma once
#include "Common.h"
#include "DX/DXDebugLayer.h"
#include "DX/DXContext.h"
#include "DX/DXCompiler.h"
#include "DX/DXWindow.h"

std::shared_ptr<DXDebugLayer> CreateDebugLayer(const GRAPHICS_DEBUGGER_TYPE gd_type);
std::shared_ptr<DXContext> CreateDXContext();
std::shared_ptr<DXCompiler> CreateDXCompiler(const std::string& directory);
std::shared_ptr<DXWindow> CreateDXWindow(const DXContext& dx_context, State* state, const std::string& window_name);