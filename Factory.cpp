#include "Factory.h"

#include "DX/DXContext.h"
#if defined(_DEBUG)
#include "DX/DXDebugLayer.h"
#else
#include "DX/DXNullDebugLayer.h"
#endif
#include "dxcapi.h" // DXC compiler TODO remove include

std::shared_ptr<DXDebugLayer> CreateDebugLayer(const GRAPHICS_DEBUGGER_TYPE gd_type)
{
	std::shared_ptr<DXDebugLayer> dx_debug_layer{};
	dx_debug_layer = std::make_shared<DXDebugLayer>(gd_type);
	dx_debug_layer->Init();
	return dx_debug_layer;
}

std::shared_ptr<DXContext> CreateDXContext()
{
	std::shared_ptr<DXContext> dx_context{};
	dx_context = std::make_shared<DXContext>();
	dx_context->Init();
	return dx_context;
}

std::shared_ptr<DXCompiler> CreateDXCompiler(const std::string& directory)
{
	std::shared_ptr<DXCompiler> dx_compiler = std::make_shared<DXCompiler>();
	dx_compiler->Init(directory);
	return dx_compiler;
}

std::shared_ptr<DXWindow> CreateDXWindow(const DXContext& dx_context, State* state, const std::string& window_name)
{
	return std::make_shared<DXWindow>(dx_context, state, window_name);
}