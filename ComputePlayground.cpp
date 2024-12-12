#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

#include "core/Common.h"
#include "DX/DXCommon.h"

#include "DX/DXWindow.h"
#include "DX/DXWindowManager.h"
#include "DX/DXCommon.h"
#include "DX/DXResource.h"
#include "DX/DXCompiler.h"
#include "DX/DXContext.h"
#include "DX/DXQuery.h"
#include "core/GPUCapture.h"
#include "DX/PSO.h"
#include "maths/LinearAlgebra.h"

#include <pix3.h>

AGILITY_SDK_DECLARE()

DISABLE_OPTIMISATIONS()
#include <chrono>
std::chrono::steady_clock::time_point start_time;

#pragma region GRAPHICS
struct ComputeResources
{
	// Compute
	Shader m_compute_shader;
	RootSignature m_compute_root_signature;
	PSO m_pso;
};

void ComputeWork
(
	DXContext& dx_context, 
	ComputeResources& compute_resource, 
	DXWindow& dx_window,
	DXResource& output_resource
);

void CreateComputeResources(DXContext& dx_context, const DXCompiler& dx_compiler, ComputeResources& resource);

struct GraphicsResources
{
	// Graphics
	// Persistent resource
	DXVertexBufferResource m_vertex_buffer;
	Shader m_vertex_shader;
	Shader m_pixel_shader;
	RootSignature m_gfx_root_signature;
	PSO m_pso;
};

void CreateGraphicsResources
(
	DXContext& dx_context, const DXCompiler& dx_compiler, const DXWindow& dx_window,
	GraphicsResources& resource
)
{
	resource.m_vertex_shader = dx_compiler.Compile(dx_context.GetDevice(), { ShaderType::VERTEX_SHADER, "VertexShader.hlsl", "main" });
	resource.m_pixel_shader = dx_compiler.Compile(dx_context.GetDevice(), {ShaderType::PIXEL_SHADER, "PixelShader.hlsl", "main" });

	{
		struct Vertex
		{
			float2 position;
			float3 color;
		};

		const Vertex vertex_data[] =
		{
			{ {0.0f, +0.25f}, {1.0f, 0.0f, 0.0f} },
			{ {+0.25f, -0.25f}, {0.0f, 1.0f, 0.0f} },
			{ {-0.25f, -0.25f}, {0.0f, 0.0f, 1.0f} },
		};

		resource.m_vertex_buffer.SetResourceInfo(D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE, sizeof(vertex_data), sizeof(vertex_data[0]));
		resource.m_vertex_buffer.CreateResource(dx_context, "VertexBuffer");

		DXResource vertex_upload_buffer{};
		vertex_upload_buffer.SetResourceInfo(D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE, sizeof(vertex_data));
		vertex_upload_buffer.m_resource_state = D3D12_RESOURCE_STATE_GENERIC_READ;
		// Memory resident in upload needs to be in generic read
		vertex_upload_buffer.CreateResource(dx_context, "VertexUploadBuffer");

		Vertex* data = nullptr;
		vertex_upload_buffer.m_resource->Map(0, nullptr, reinterpret_cast<void**>(&data)) >> CHK;
		memcpy(data, vertex_data, sizeof(vertex_data));
		vertex_upload_buffer.m_resource->Unmap(0, nullptr);
		dx_context.InitCommandLists();
		// Technically, dont need to transition to copy source because generic is copy source and more or'd together
		dx_context.Transition(D3D12_RESOURCE_STATE_COPY_SOURCE, vertex_upload_buffer);
		dx_context.Transition(D3D12_RESOURCE_STATE_COPY_DEST, resource.m_vertex_buffer);
		dx_context.GetCommandListGraphics()->CopyResource(resource.m_vertex_buffer.m_resource.Get(), vertex_upload_buffer.m_resource.Get());
		dx_context.Transition(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, resource.m_vertex_buffer);
		// Copy queue has some constraints regarding copy state and barriers
		// Compute has synchronization issue
		dx_context.ExecuteCommandListGraphics();
		dx_context.Flush(1);

		// Same rootsignature for VS and PS
		resource.m_gfx_root_signature = dx_context.CreateRS(resource.m_pixel_shader);
		
		std::string graphics_name { "Graphics" };
		std::wstring graphics_wname = std::to_wstring(graphics_name);

		// Need to export entry point with generic program
		// Entry point needs to be differentiate between VS and PS
		std::wstring common_entrypoint = L"main";
		std::wstring vertexshader_entrypoint = L"mainVS";
		std::wstring pixelshader_entrypoint = L"mainPS";

		CD3DX12_STATE_OBJECT_DESC cstate_object_desc;
		cstate_object_desc.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);

		CD3DX12_DXIL_LIBRARY_SUBOBJECT* vs_subobj = cstate_object_desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
		D3D12_SHADER_BYTECODE vs_byte_code = BlobToByteCode(resource.m_vertex_shader.m_blob);
		vs_subobj->SetDXILLibrary(&vs_byte_code);
		vs_subobj->DefineExport(vertexshader_entrypoint.c_str(), common_entrypoint.c_str());

		CD3DX12_DXIL_LIBRARY_SUBOBJECT* ps_subobj = cstate_object_desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
		D3D12_SHADER_BYTECODE ps_byte_code = BlobToByteCode(resource.m_pixel_shader.m_blob);
		ps_subobj->SetDXILLibrary(&ps_byte_code);
		ps_subobj->DefineExport(pixelshader_entrypoint.c_str(), common_entrypoint.c_str());
		
		CD3DX12_PRIMITIVE_TOPOLOGY_SUBOBJECT* topology_subobj = cstate_object_desc.CreateSubobject<CD3DX12_PRIMITIVE_TOPOLOGY_SUBOBJECT>();
		topology_subobj->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		CD3DX12_RENDER_TARGET_FORMATS_SUBOBJECT* rt_subobj = cstate_object_desc.CreateSubobject<CD3DX12_RENDER_TARGET_FORMATS_SUBOBJECT>();
		rt_subobj->SetNumRenderTargets(1);
		rt_subobj->SetRenderTargetFormat(0, dx_window.GetFormat());

		CD3DX12_GENERIC_PROGRAM_SUBOBJECT* generic_subobj = cstate_object_desc.CreateSubobject<CD3DX12_GENERIC_PROGRAM_SUBOBJECT>();
		generic_subobj->SetProgramName(graphics_wname.c_str());
		generic_subobj->AddExport(vertexshader_entrypoint.c_str());
		generic_subobj->AddExport(pixelshader_entrypoint.c_str());
		generic_subobj->AddSubobject(*topology_subobj);
		generic_subobj->AddSubobject(*rt_subobj);
		// Seem like the SO still need RS, even when its embedded in the shader, in contrast to PSO
		CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT* global_rootsignature_subobj = cstate_object_desc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
		global_rootsignature_subobj->SetRootSignature(resource.m_gfx_root_signature.m_signature.Get());
		// TODO: CD3DX12 cause PIX to fail on CreateSO
		resource.m_pso = CreatePSO(dx_context, cstate_object_desc, graphics_name);
	}
}

void GraphicsWork
(
	DXContext& dx_context, DXWindow& dx_window, 
	GraphicsResources& resource, 
	DXTextureResource& output_resource
)
{
	// Draw Work
	{
		D3D12_VIEWPORT view_ports[] =
		{
			{
				.TopLeftX = 0,
				.TopLeftY = 0,
				.Width = (float)dx_window.GetWidth(),
				.Height = (float)dx_window.GetHeight(),
				.MinDepth = 0,
				.MaxDepth = 1,
			}
		};
		D3D12_RECT scissor_rects[] =
		{
			{
				.left = 0,
				.top = 0,
				.right = (long)dx_window.GetWidth(),
				.bottom = (long)dx_window.GetHeight(),
			}
		};
		dx_context.GetCommandListGraphics()->RSSetViewports(1, view_ports);
		dx_context.GetCommandListGraphics()->RSSetScissorRects(1, scissor_rects);
		
		D3D12_SHADER_RESOURCE_VIEW_DESC desc = GetStructuredBufferSRVDesc(resource.m_vertex_buffer.m_count, resource.m_vertex_buffer.m_stride);
		SRV srv = dx_context.CreateSRV(resource.m_vertex_buffer, desc);

		dx_context.GetCommandListGraphics()->SetGraphicsRootSignature(resource.m_gfx_root_signature.m_signature.Get());

		D3D12_SET_PROGRAM_DESC program_desc
		{
			.Type = D3D12_PROGRAM_TYPE_GENERIC_PIPELINE,
			.GenericPipeline =
			{
				.ProgramIdentifier = resource.m_pso.m_program_id,
			},
		};
		dx_context.GetCommandListGraphics()->SetProgram(&program_desc);
		uint32 bindless_index = srv.m_bindless_index;
		dx_context.GetCommandListGraphics()->SetGraphicsRoot32BitConstants(0, 1, &bindless_index, 0);
		dx_context.GetCommandListGraphics()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		// transition
		
		dx_context.Transition(D3D12_RESOURCE_STATE_RENDER_TARGET, output_resource);
		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc
		{
			.Format = output_resource.m_format,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
			.Texture2D =
			{
				.MipSlice = 0,
				.PlaneSlice = 0,
			},
		};
		ID3D12Resource* d3d12_ouput_resource = output_resource.m_resource.Get();
		dx_context.OMSetRenderTargets(1, &d3d12_ouput_resource, &rtv_desc, nullptr, nullptr);
		dx_context.GetCommandListGraphics()->DrawInstanced(resource.m_vertex_buffer.m_count, 16, 0, 0);
		//dx_context.GetCommandListGraphics()->DrawInstanced(3, 1, 0, 0);
		//dx_context.GetCommandListGraphics()->DrawIndexedInstanced(3, 1, 0, 0, 0);
	}
}

void FillCommandList
(
	DXContext& dx_context, DXWindow& dx_window, 
	GraphicsResources& gfx_resource,
	ComputeResources& compute_resource
)
{
	// Set descriptor heap before root signature, order required by spec
	dx_context.GetCommandListGraphics()->SetDescriptorHeaps(1, dx_context.m_resources_descriptor_heap.m_heap.GetAddressOf());
	{
		ComputeWork(dx_context, compute_resource, dx_window, dx_window.m_buffers[g_current_buffer_index]);
		//GraphicsWork(dx_context, dx_window, gfx_resource, dx_window.m_buffers[g_current_buffer_index]);
	}
}

#include <chrono>
#include <thread>

class UI
{
public:
	~UI()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
	void Init(DXContext& dx_context, DXTextureResource& output, HWND window_handle)
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

		// Setup ImGui DX resources
		dx_context.CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, "ImGui descriptor heap", m_imgui_descriptor_heap);
		// No bindless
		DXDescriptor descriptor{};
		descriptor.m_cpu_descriptor_handle = dx_context.GetCPUDescriptorHandle(m_imgui_descriptor_heap, 0);
		descriptor.m_gpu_descriptor_handle = dx_context.GetGPUDescriptorHandle(m_imgui_descriptor_heap, 0);

		ImGui_ImplWin32_Init(window_handle);
		ImGui_ImplDX12_Init
		(
			dx_context.GetDevice().Get(), g_backbuffer_count, output.m_format,
		    m_imgui_descriptor_heap.m_heap.Get(),
		    descriptor.m_cpu_descriptor_handle,
		    descriptor.m_gpu_descriptor_handle
		);
	}

	void ImGUI(DXContext& dx_context)
	{
		ImGui::ShowDemoWindow(); // Show demo window! :)
		auto [bytes_used, bytes_budget] = GetVRAM(dx_context.m_adapter);
		ImGui::Text("VRAM usage: %d MB / %d MB", ToMB(bytes_used), ToMB(bytes_budget));
		auto [system_bytes_used, system_bytes_budget] = GetSystemRAM(dx_context.m_adapter);
		ImGui::Text("System RAM usage: %d MB / %d MB", ToMB(system_bytes_used), ToMB(system_bytes_budget));
	}

	void FillcommandlistImGui(DXContext& dx_context, DXTextureResource& output)
	{
		dx_context.Transition(D3D12_RESOURCE_STATE_RENDER_TARGET, output);
		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc
		{
			.Format = output.m_format,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
			.Texture2D =
			{
				.MipSlice = 0,
				.PlaneSlice = 0,
			},
		};
		ID3D12Resource* d3d12_ouput_resource = output.m_resource.Get();
		dx_context.OMSetRenderTargets(1, &d3d12_ouput_resource, &rtv_desc, nullptr, nullptr);
		dx_context.GetCommandListGraphics()->SetDescriptorHeaps(1, m_imgui_descriptor_heap.m_heap.GetAddressOf());
		// Actual GPU draw commands
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dx_context.GetCommandListGraphics().Get());
	}

	void Render(DXContext& dx_context, DXTextureResource& output)
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGUI(dx_context);
		ImGui::Render();
		FillcommandlistImGui(dx_context, output);

	}
	DescriptorHeap m_imgui_descriptor_heap;
};

void RunWindowLoop(DXContext& dx_context, DXCompiler& dx_compiler, GPUCapture* gpu_capture)
{
	DXWindowManager window_manager;
	{
		uint32 width = 1500;
		uint32 height = 800;
		WindowDesc window_desc =
		{
			.m_window_name = { "Playground" },
			.m_width = width,
			.m_height = height,
			.m_origin_x = (1920 >> 1) - (width >> 1),
			.m_origin_y = (1080 >> 1) - (height >> 1),
		};
		DXWindow dx_window(dx_context, window_manager, window_desc);
		UI ui;
		ui.Init(dx_context, dx_window.m_buffers[g_current_buffer_index], dx_window.m_handle);
		{
			GraphicsResources gfx_resource{};
			//CreateGraphicsResources(dx_context, dx_compiler, dx_window, gfx_resource);
			ComputeResources compute_resource{};
			CreateComputeResources(dx_context, dx_compiler, compute_resource);
			while (!dx_window.ShouldClose())
			{
				std::chrono::milliseconds ms(8);
				std::this_thread::sleep_for(ms);
				// Process window message
				dx_window.Update();

				// Key input handling of application
				static bool prev_F1_pressed = false;
				bool capture = false;
				bool current_F1_pressed = dx_window.input.IsKeyPressed(VK_F1);
				if (current_F1_pressed && prev_F1_pressed != current_F1_pressed)
				{
					capture = true;
				}
				prev_F1_pressed = current_F1_pressed;
					
				if (dx_window.input.IsKeyPressed(VK_ESCAPE))
				{
					dx_window.m_should_close = true;
				}

				// TODO support release button essentially needed
				static bool prev_F11_pressed = false;
				bool current_F11_pressed = dx_window.input.IsKeyPressed(VK_F11);
				if (current_F11_pressed && prev_F11_pressed != current_F11_pressed)
				{
					// F11 also sends a WM_SIZE after
					WindowMode current_window_mode = dx_window.GetWindowModeRequest();
					WindowMode next_window_mode = static_cast<WindowMode>((static_cast<int32>(current_window_mode) + 1) % (static_cast<int32>(WindowMode::Count)));

					dx_window.SetWindowModeRequest(next_window_mode);
					//dx_window.ToggleWindowMode();
				}
				prev_F11_pressed = current_F11_pressed;

				if (capture && gpu_capture != nullptr)
				{
					gpu_capture->StartCapture();
				}
				// rendering
				{
					// Handle resizing
					if (dx_window.ShouldResize())
					{
						dx_context.Flush(dx_window.GetBackBufferCount());
						dx_window.Resize();
						// Seems like back buffer index needs to be updated on resize
						// Always sets it back 0
						dx_window.UpdateBackBufferIndex();
					}

					{
						dx_context.InitCommandLists();
						{
							PIXScopedEvent(dx_context.GetCommandListGraphics().Get(), 0, "Frame");
							dx_window.BeginFrame(dx_context);
							{
								PIXScopedEvent(dx_context.GetCommandListGraphics().Get(), 0, "FillCommandList");
								FillCommandList(dx_context, dx_window, gfx_resource, compute_resource);
							}
							{
								PIXScopedEvent(dx_context.GetCommandListGraphics().Get(), 0, "ImGui");
								ui.Render(dx_context, dx_window.m_buffers[g_current_buffer_index]);
							}
							dx_window.EndFrame(dx_context);
						}
						dx_context.ExecuteCommandListGraphics();
						dx_window.Present(dx_context);
					}
				}
				if (capture && gpu_capture != nullptr)
				{
					gpu_capture->EndCapture();
					gpu_capture->OpenCapture();
				}

			}
			dx_context.Flush(dx_window.GetBackBufferCount());
		}


	}
}
#pragma endregion

#pragma region COMPUTE
void CreateComputeResources(DXContext& dx_context, const DXCompiler& dx_compiler, ComputeResources& resource)
{
	resource.m_compute_shader = dx_compiler.Compile(dx_context.GetDevice(), { ShaderType::COMPUTE_SHADER, "ComputeShader.hlsl", "main" });

	// Root signature embed in the shader
	resource.m_compute_root_signature = dx_context.CreateRS(resource.m_compute_shader);

	std::string compute_name{ "Compute" };
	std::wstring compute_wname = std::to_wstring(compute_name);

	CD3DX12_STATE_OBJECT_DESC cstate_object_desc;
	cstate_object_desc.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);
	CD3DX12_DXIL_LIBRARY_SUBOBJECT* cs_subobj = cstate_object_desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	D3D12_SHADER_BYTECODE cs_byte_code = BlobToByteCode(resource.m_compute_shader.m_blob);
	cs_subobj->SetDXILLibrary(&cs_byte_code);
	CD3DX12_GENERIC_PROGRAM_SUBOBJECT* generic_subobj = cstate_object_desc.CreateSubobject<CD3DX12_GENERIC_PROGRAM_SUBOBJECT>();
	generic_subobj->SetProgramName(compute_wname.c_str());
	generic_subobj->AddExport(L"main");
	// Apparently works without linking SO with RS, though RS is embedded in shader already but seem to be needed for graphics SO
	resource.m_pso = CreatePSO(dx_context, cstate_object_desc, compute_name);
}

void ComputeWork
(
	DXContext& dx_context, 
	ComputeResources& compute_resource, 
	DXWindow& dx_window,
	DXResource& output_resource
)
{
	DXTextureResource gpu_resource;
	gpu_resource.SetResourceInfo(D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, dx_window.GetWidth(), dx_window.GetHeight(), output_resource.m_resource_desc.Format);
	gpu_resource.CreateResource(dx_context, "GPU resource");
	
	dx_context.m_resource_handler.RegisterResource(gpu_resource);

	struct MyCBuffer
	{
		uint2 resolution;
		float32 iTime;
		uint32 iFrame;
		uint32 bindless_index;
	}; 

	// Compute Work
	// Transition to UAV
	dx_context.Transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, gpu_resource);

	D3D12_SET_PROGRAM_DESC program_desc
	{
		.Type = D3D12_PROGRAM_TYPE_GENERIC_PIPELINE,
		.GenericPipeline =
		{
			.ProgramIdentifier = compute_resource.m_pso.m_program_id
		},
	};
	dx_context.GetCommandListGraphics()->SetProgram(&program_desc);

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAV_desc = GetTexture2DUAVDesc(gpu_resource.m_format);
	UAV uav = dx_context.CreateUAV(gpu_resource, UAV_desc);
	
	auto current_time = std::chrono::high_resolution_clock::now();
	auto diff_seconds = (float32)std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
	diff_seconds /= 1000;
	static uint32 iFrame = 0;

	MyCBuffer cbuffer
	{
		.resolution = {(uint32)gpu_resource.m_resource_desc.Width, (uint32)gpu_resource.m_resource_desc.Height},
		.iTime = diff_seconds,
		.iFrame = iFrame,
		.bindless_index = uav.m_bindless_index
	};
	++iFrame;
	dx_context.GetCommandListGraphics()->SetComputeRootSignature(compute_resource.m_compute_root_signature.m_signature.Get());
	
	dx_context.GetCommandListGraphics()->SetComputeRoot32BitConstants(0, sizeof(MyCBuffer) / 4, &cbuffer, 0);
	uint32 dispatch_x = DivideRoundUp(gpu_resource.m_width, 8);
	uint32 dispatch_y = DivideRoundUp(gpu_resource.m_height, 8);
	dx_context.GetCommandListGraphics()->Dispatch(dispatch_x, dispatch_y, 1);
	// Transition to Copy Src
	dx_context.Transition(D3D12_RESOURCE_STATE_COPY_DEST, output_resource);
	dx_context.Transition(D3D12_RESOURCE_STATE_COPY_SOURCE, gpu_resource);
	dx_context.GetCommandListGraphics()->CopyResource(output_resource.m_resource.Get(), gpu_resource.m_resource.Get());
}

void RunComputeWork(DXContext& dx_context)
{
	dx_context.InitCommandLists();
	dx_context.ExecuteCommandListGraphics();

#if defined(TEST_READBACK_COMPUTE)
	dx_context.Flush(1);
	float32* data = nullptr;
	const D3D12_RANGE range = { 0, resource.m_gpu_resource.m_resource_desc.Width };
	resource.m_cpu_resource.m_resource->Map(0, &range, (void**)&data);
	bool has_display = false;
	if (!has_display)
	{
		for (uint32 i = 0; i < resource.m_cpu_resource.m_resource_desc.Width / sizeof(float32) / 4; i++)
		{
			LogTrace("uav[{}] = {}, {}, {}, {}\n", i, data[i * 4 + 0], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]);
		}
		has_display = true;
	}
	resource.m_cpu_resource.m_resource->Unmap(0, nullptr);
#endif
}
#pragma endregion

#pragma region WORKGRAPH
struct WorkGraphResources
{
	// Compute
	DXResource m_scratch_buffer;
	DXResource m_gpu_buffer;
	DXResource m_cpu_buffer;

	Shader m_workgraph_shader;
	RootSignature m_workgraph_root_signature;
	//Microsoft::WRL::ComPtr<ID3D12StateObject> m_workgraph_so;
	PSO m_pso;
};

void CreateWorkGraphResource
 (
	 DXContext& dx_context, DXCompiler& dx_compiler, 
	 WorkGraphResources& resource,
	 bool is_pix_running
 )
{
	resource.m_workgraph_shader = dx_compiler.Compile(dx_context.GetDevice(), { ShaderType::LIB_SHADER, "WorkGraphShader.hlsl", "main" });

	D3D12_SHADER_BYTECODE byte_code = BlobToByteCode(resource.m_workgraph_shader.m_blob);

	resource.m_workgraph_root_signature = dx_context.CreateRS(resource.m_workgraph_shader);

	CD3DX12_STATE_OBJECT_DESC cstate_object_desc;
	cstate_object_desc.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);

	CD3DX12_DXIL_LIBRARY_SUBOBJECT* cs_subobj = cstate_object_desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	cs_subobj->SetDXILLibrary(&byte_code);
	CD3DX12_WORK_GRAPH_SUBOBJECT* workgraph_subobj = cstate_object_desc.CreateSubobject<CD3DX12_WORK_GRAPH_SUBOBJECT>();
	std::string work_graph_name{ "WorkGraph" };
	std::wstring work_graph_wname = std::to_wstring(work_graph_name);
	workgraph_subobj->SetProgramName(work_graph_wname.c_str());
	workgraph_subobj->IncludeAllAvailableNodes();
	// Apparently meeds linking SO with RS, though RS is embedded in shader already but seem to be needed for graphics SO
	CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT* rootsignature_subobj = cstate_object_desc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	rootsignature_subobj->SetRootSignature(resource.m_workgraph_root_signature.m_signature.Get());

	resource.m_pso = CreatePSO(dx_context, cstate_object_desc, work_graph_name);

	// These are not subtypes but you can QueryInterface as how ComPtr works apparently
	// To figure out from which to what you can QueryInterface, look up the docs they said ..
	Microsoft::WRL::ComPtr<ID3D12WorkGraphProperties> workgraph_properties;
	resource.m_pso.m_so.As(&workgraph_properties) >> CHK;
	uint32 workgraph_index = workgraph_properties->GetWorkGraphIndex(work_graph_wname.c_str());
	D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS mem_requirements{};
	workgraph_properties->GetWorkGraphMemoryRequirements(workgraph_index, &mem_requirements);

	if (mem_requirements.MaxSizeInBytes > 0)
	{
		// For some reason running with PIX requires UAV
		// Probably PIX needs to read the resource
		resource.m_scratch_buffer.SetResourceInfo(D3D12_HEAP_TYPE_DEFAULT, is_pix_running ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE, (uint32)mem_requirements.MaxSizeInBytes);
		resource.m_scratch_buffer.CreateResource(dx_context, "Scratch Memory");
	}

	resource.m_gpu_buffer.SetResourceInfo(D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 16777216 * sizeof(uint32));
	resource.m_gpu_buffer.CreateResource(dx_context, "Buffer UAV resource");

	resource.m_cpu_buffer.SetResourceInfo(D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_FLAG_NONE, (uint32)resource.m_gpu_buffer.m_resource_desc.Width);
	resource.m_cpu_buffer.m_resource_state = D3D12_RESOURCE_STATE_COPY_DEST;
	resource.m_cpu_buffer.CreateResource(dx_context, "Readback resource");
}

void RunWorkGraph(DXContext& dx_context, DXCompiler& dx_compiler, bool is_pix_running = false)
{
	WorkGraphResources resource{};
	CreateWorkGraphResource(dx_context, dx_compiler, resource, is_pix_running);

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE backing_memory{};
	if (resource.m_scratch_buffer.m_resource)
	{
		backing_memory.SizeInBytes = resource.m_scratch_buffer.m_size_in_bytes;
		backing_memory.StartAddress = resource.m_scratch_buffer.m_resource->GetGPUVirtualAddress();
	}

	D3D12_SET_PROGRAM_DESC program_desc =
	{
		.Type = D3D12_PROGRAM_TYPE_WORK_GRAPH,
		.WorkGraph =
		{
			.ProgramIdentifier = resource.m_pso.m_program_id,
			.Flags = D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE,
			.BackingMemory = backing_memory,
			.NodeLocalRootArgumentsTable = 0,
		},
	};
//#define WORKGRAPH_TEST1
//#define WORKGRAPH_TEST2
//#define WORKGRAPH_TEST3
//#define WORKGRAPH_TEST4
//#define WORKGRAPH_TEST5
//#define WORKGRAPH_TEST6
#if defined(WORKGRAPH_TEST1)
	struct Record
	{
		uint32 index;
	};
	std::vector<Record> records{};
	//records.push_back(Record{ 0 });
	//records.push_back(Record{ 1 });
#elif defined(WORKGRAPH_TEST2)
	struct Record
	{
		uint32 index;
	};
	std::vector<Record> records{};
	records.push_back(Record{ 0 });
	records.push_back(Record{ 1 });
#elif defined(WORKGRAPH_TEST3)
	struct Record
	{
		uint32 DispatchGrid[3];
		uint32 index;
	};
	std::vector<Record> records{};
	records.push_back( {{ 1, 2, 3 }, 0});
	records.push_back( {{ 2, 2, 2 }, 1});
#elif defined(WORKGRAPH_TEST4)
	struct Record
	{
		uint32 index;
	};
	std::vector<Record> records{};
	records.push_back(Record{ 0 });
	records.push_back(Record{ 1 });
	records.push_back(Record{ 2 });
#elif defined(WORKGRAPH_TEST5)
	struct Record
	{
		uint32 index;
	};
	std::vector<Record> records{};
	records.push_back(Record{ 0 });
	records.push_back(Record{ 1 });
	//records.push_back(Record{ 2 });
#elif defined(WORKGRAPH_TEST6)
	struct Record
	{
		uint32 index;
	};
	std::vector<Record> records{};
	records.push_back(Record{ 0 });
	//records.push_back(Record{ 1 });
	//records.push_back(Record{ 2 });
#else

	struct Record
	{
		uint32 gridSize; // : SV_DispatchGrid;
		uint32 recordIndex;
	};
	std::vector<Record> records{};
//	records.push_back(Record{ 1, 0 });
	records.push_back(Record{ 2, 1 });
//	records.push_back(Record{ 3, 2 });
//	records.push_back(Record{ 4, 3 });
#endif

	D3D12_DISPATCH_GRAPH_DESC workgraph_desc =
	{
		.Mode = D3D12_DISPATCH_MODE_NODE_CPU_INPUT,
		.NodeCPUInput =
		{
			.EntrypointIndex = 0,
			.NumRecords = std::max((uint32)records.size(), 1u), // Needs at least 1 to dispatch work graph
			.pRecords = records.data(),
			.RecordStrideInBytes = sizeof(Record)
		},
	};

	dx_context.InitCommandLists();
	dx_context.GetCommandListGraphics()->SetComputeRootSignature(resource.m_workgraph_root_signature.m_signature.Get());
	dx_context.GetCommandListGraphics()->SetComputeRootUnorderedAccessView(0, resource.m_gpu_buffer.m_resource->GetGPUVirtualAddress());
	dx_context.GetCommandListGraphics()->SetProgram(&program_desc);
	dx_context.GetCommandListGraphics()->DispatchGraph(&workgraph_desc);
	LogTrace("Workgraph Dispatched");

	dx_context.Transition(D3D12_RESOURCE_STATE_COPY_SOURCE, resource.m_gpu_buffer);
	dx_context.GetCommandListGraphics()->CopyResource(resource.m_cpu_buffer.m_resource.Get(), resource.m_gpu_buffer.m_resource.Get());
	dx_context.ExecuteCommandListGraphics();
	dx_context.Flush(1);

	uint32* data = nullptr;
	const D3D12_RANGE range = { 0, resource.m_cpu_buffer.m_resource_desc.Width };
	resource.m_cpu_buffer.m_resource->Map(0, &range, (void**)&data);
	resource.m_cpu_buffer.m_resource->Unmap(0, nullptr);
	LogTrace("Readback from GPU");

	for (uint32 i = 0; i < 33; ++i)
	{
		LogTrace("uav workgraph[{}] = {}\n", i, data[i]);
	}
}
#pragma endregion

std::shared_ptr<GPUCapture> CreateGPUCapture()
{
	//return nullptr;
	return std::make_shared<PIXCapture>();
	// RenderDoc doesnt support WorkGraph and newer interfaces
	//return std::make_shared<RenderDocCapture>();
}

int main()
{
	MemoryTrack();
	DXReportContext dx_report_context{};
	{
		std::shared_ptr<GPUCapture> gpu_capture = CreateGPUCapture();
		DXContext dx_context{};
		dx_report_context.SetDevice(dx_context.GetDevice(), dx_context.m_adapter);
		DXCompiler dx_compiler("shaders");
		//RunWorkGraph(dx_context, dx_compiler, dynamic_cast<PIXCapture*>(gpu_capture) != nullptr);
		RunWindowLoop(dx_context, dx_compiler, gpu_capture.get());
	}
	
	return 0;
}