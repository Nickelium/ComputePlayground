#include "Factory.h"

#include "DX/DXContext.h"
#if defined(_DEBUG)
#include "DX/DXDebugContext.h"
#include "DX/DXDebugLayer.h"
#else
#include "DX/DXNullDebugLayer.h"
#endif
#include "dxcapi.h" // DXC compiler TODO remove include

std::shared_ptr<IDXDebugLayer> CreateDebugLayer(GRAPHICS_DEBUGGER_TYPE gd_type)
{
	std::shared_ptr<IDXDebugLayer> dx_debug_layer{};
#if defined(_DEBUG)
	dx_debug_layer = std::make_shared<DXDebugLayer>(gd_type);
#else
	UNUSED(gd_type);
	dx_debug_layer = std::make_shared<DXNullDebugLayer>();
#endif
	dx_debug_layer->Init();
	return dx_debug_layer;
}

std::shared_ptr<DXContext> CreateDXContext(GRAPHICS_DEBUGGER_TYPE gd_type)
{
	std::shared_ptr<DXContext> dx_context{};
#if defined(_DEBUG)
	dx_context = std::make_shared<DXDebugContext>(gd_type == GRAPHICS_DEBUGGER_TYPE::RENDERDOC);
#else
	UNUSED(gd_type);
	dx_context = std::make_shared<DXContext>();
#endif
	dx_context->Init();
	return dx_context;
}

std::shared_ptr<DXCompiler> CreateDXCompiler()
{
	std::shared_ptr<DXCompiler> dx_compiler = std::make_shared<DXCompiler>();
	bool compile_debug{};
#if defined(_DEBUG)
	compile_debug = true;
#else
	compile_debug = false;
#endif
	dx_compiler->Init(compile_debug);
	return dx_compiler;
}

std::shared_ptr<DXWindow> CreateDXWindow(const DXContext& dx_context, State* state, const std::string& window_name)
{
	return std::make_shared<DXWindow>(dx_context, state, window_name);
}