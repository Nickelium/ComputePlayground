#include "DXDebugLayer.h"
#include "../Common.h"
#include "DXCommon.h"
#include <dxgidebug.h>

#define USE_PIX
#include <pix3.h>
#include "renderdoc/inc/renderdoc_app.h"

bool LoadRenderdoc(HMODULE* renderdoc_module_out, RENDERDOC_API_1_6_0** renderdoc_api_out)
{
	RENDERDOC_API_1_6_0* renderdoc_api = nullptr;
	HMODULE renderdoc_module = 0;

	TCHAR current_directory[MAX_PATH + 1] = { 0 };
	const DWORD number_characters_written = ::GetCurrentDirectory(MAX_PATH, current_directory);
	ASSERT(number_characters_written != 0 && "Failed current directory, call GetLastError");
	const std::wstring renderdoc_dll_name = L"renderdoc.dll";
	const std::wstring renderdoc_dll_relative_path = L"\\dependencies\\renderdoc\\bin\\";
	std::wstring renderdoc_dll_absolute_path = current_directory;
	renderdoc_dll_absolute_path += renderdoc_dll_relative_path;
	renderdoc_dll_absolute_path += renderdoc_dll_name;
	renderdoc_module = LoadLibraryExW(renderdoc_dll_absolute_path.c_str(), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	ASSERT(renderdoc_module && "Missing renderdoc dependencies");
	pRENDERDOC_GetAPI renderdoc_get_API =
		(pRENDERDOC_GetAPI)GetProcAddress(renderdoc_module, "RENDERDOC_GetAPI");
	const int ret = renderdoc_get_API(eRENDERDOC_API_Version_1_6_0, (void**)&renderdoc_api);
	ASSERT(ret == 1);

	*renderdoc_module_out = renderdoc_module;
	*renderdoc_api_out = renderdoc_api;

	return true;
}

bool LoadPIX(HMODULE* pix_module_out)
{
	// Note: WinPixEventRuntime != WinPixGpuCapturer
	// To GPU capture we need both
	// To actually open the capture, the user need to install PIX themselves

	HMODULE pix_module = 0;

	TCHAR current_directory[MAX_PATH + 1] = { 0 };
	const DWORD number_characters_written = ::GetCurrentDirectory(MAX_PATH, current_directory);
	ASSERT(number_characters_written != 0 && "Failed current directory, call GetLastError");
	const std::wstring pix_dll_name = L"WinPixGpuCapturer.dll";
	const std::wstring pix_dll_relative_path = L"\\dependencies\\pix\\bin\\";
	std::wstring pix_dll_absolute_path = current_directory;
	pix_dll_absolute_path += pix_dll_relative_path;
	pix_dll_absolute_path += pix_dll_name;
	pix_module = LoadLibraryExW(pix_dll_absolute_path.c_str(), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	ASSERT(pix_module && "Missing PIX dependencies");
	*pix_module_out = pix_module;

	return true;
}

#include <filesystem>
std::string GetNewestCaptureName(const std::string& capture_relative_path, const std::string& capture_template_name, const std::string& capture_extension)
{
	std::string identifier = "";
	std::string capture_absolute_path =
		capture_relative_path + capture_template_name + identifier + capture_extension;
	// 2 to match renderdoc name capturing behaviour
	uint32_t index = 2;
	while (std::filesystem::exists(capture_absolute_path))
	{
		identifier = "_" + std::to_string(index);
		capture_absolute_path =
			capture_relative_path + capture_template_name + identifier + capture_extension;
		++index;
	}
	return capture_absolute_path;
}

void DXDebugLayer::RenderdocCaptureStart()
{
	if (m_renderdoc_api)
	{
		m_renderdoc_api->StartFrameCapture(nullptr, nullptr);
	}
}

#include <regex>
std::vector<std::string> GetAllFileNames(const std::string& path)
{
	std::vector<std::string> file_names;
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		std::string file_name = entry.path().filename().string();
		file_names.push_back(file_name);
	}
	return file_names;
}

std::vector<int32_t> GetRegexIndexOfFileNames(const std::vector<std::string>& file_names, const std::string& regex_string)
{
	std::vector<int32_t> index_array;

	std::regex reg(regex_string);
	for (int32 i = 0; i < file_names.size(); ++i)
	{
		std::string test = file_names[i];
		std::cmatch match;
		std::regex_match(test.c_str(), match, reg);
		if (match.size() > 0)
		{
			std::string string_match = match.str(1);
			int32 index_match = std::stoi(string_match);
			index_array.push_back(index_match);
		}
	}

	return index_array;
}

void DXDebugLayer::RenderdocCaptureEnd()
{
	if (!m_renderdoc_api)
		return;

	const std::string path = ".\\captures\\";
	const std::vector<std::string> file_names = GetAllFileNames(path);
	const std::string regex_string(R"(rdc_(\d+)_capture.*)");
	std::vector<int32> index_matches = GetRegexIndexOfFileNames(file_names, regex_string);
	std::sort(index_matches.begin(), index_matches.end());
	int32 max_index = -1;
	if (index_matches.size() > 0)
	{
		max_index = index_matches.back();
	}

	const std::string file_path_template = "captures/";
	std::string name_template = "rdc";
	name_template += "_" + std::to_string(max_index + 1);
	const std::string full_path_template = file_path_template + name_template;
	m_renderdoc_api->SetCaptureFilePathTemplate(full_path_template.c_str());

	const std::string& renderdoc_relative_path = ".\\captures\\";
	const std::string& renderdoc_template_name = name_template + "_capture";
	const std::string& renderdoc_extension = ".rdc";
	const std::string& renderdoc_absolute_path = GetNewestCaptureName(renderdoc_relative_path, renderdoc_template_name, renderdoc_extension);

	m_renderdoc_api->EndFrameCapture(nullptr, nullptr);
	// TODO scope out these function calls
	const std::wstring& renderdoc_absolute_path_wstring = std::to_wstring(renderdoc_absolute_path);
	ShellExecute(0, 0, renderdoc_absolute_path_wstring.c_str(), 0, 0, SW_SHOW);
}

DXDebugLayer::DXDebugLayer(const GRAPHICS_DEBUGGER_TYPE type) :
	m_pix_module(0u),
	m_renderdoc_module(0u),
	m_renderdoc_api(nullptr),
	m_graphics_debugger_type(type)
{

}

DXDebugLayer::~DXDebugLayer()
{
	Close();
}

void DXDebugLayer::Init()
{
	_set_error_mode(_OUT_TO_STDERR);
	switch (m_graphics_debugger_type)
	{
	case GRAPHICS_DEBUGGER_TYPE::PIX:
	{
		bool success = LoadPIX(&m_pix_module);
		if (!success)
		{
			printf("PIX not installed %d\n", GetLastError());
		}
		break;
	}
	case GRAPHICS_DEBUGGER_TYPE::RENDERDOC:
	{
		const bool success = LoadRenderdoc(&m_renderdoc_module, &m_renderdoc_api);
		if (!success)
		{
			printf("RenderDoc not installed %d\n", GetLastError());
		}
		break;
	}
	default:
	{
		break;
	}
	}

	// Requires windows "Graphics Tool" optional feature
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_dxgi_debug)) >> CHK;
	m_dxgi_debug->EnableLeakTrackingForThread();

	// Dont need to store -> can scope out
	D3D12GetDebugInterface(IID_PPV_ARGS(&m_d3d12_debug)) >> CHK;
	m_d3d12_debug->EnableDebugLayer();
	m_d3d12_debug->SetEnableGPUBasedValidation(true);
	m_d3d12_debug->SetEnableAutoName(true);
	m_d3d12_debug->SetEnableSynchronizedCommandQueueValidation(true);
}

void DXDebugLayer::Close()
{
	if (m_pix_module)
	{
		const bool success = FreeLibrary(m_pix_module);
		ASSERT(success);
	}
	if (m_renderdoc_module)
	{
		const bool success = FreeLibrary(m_renderdoc_module);
		ASSERT(success);
	}

	OutputDebugStringW(L"Report Live DXGI Objects:\n");
	m_dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL)) >> CHK;

}

void DXDebugLayer::PIXCaptureAndOpen()
{
#if defined(_DEBUG)
	if (m_pix_module)
	{
		const std::string& pix_relative_path = ".\\captures\\";
		const std::string& pix_template_name = "pix_capture";
		const std::string& pix_extension = ".wpix";
		const std::string& pix_absolute_path = GetNewestCaptureName(pix_relative_path, pix_template_name, pix_extension);
		const std::wstring& pix_absolute_path_wstring = std::to_wstring(pix_absolute_path);
		PIXGpuCaptureNextFrames(pix_absolute_path_wstring.c_str(), 1) >> CHK;
		ShellExecute(0, 0, pix_absolute_path_wstring.c_str(), 0, 0, SW_SHOW);
	}
#endif
}
