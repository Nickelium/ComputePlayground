// ImGui new operator clash with something?
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
#include "core/GPUCapture.h"

#include "maths/LinearAlgebra.h"


AGILITY_SDK_DECLARE()

DISABLE_OPTIMISATIONS()
#include <chrono>
std::chrono::steady_clock::time_point start_time;

#pragma region GRAPHICS
struct ComputeResources
{
	// Compute
	Microsoft::WRL::ComPtr<IDxcBlob> m_compute_shader;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_compute_root_signature;
	
	// State object needs to be alive for the id to work
	Microsoft::WRL::ComPtr<ID3D12StateObject> m_compute_so;
	D3D12_PROGRAM_IDENTIFIER m_program_id;
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
	Microsoft::WRL::ComPtr<IDxcBlob> m_vertex_shader;
	Microsoft::WRL::ComPtr<IDxcBlob> m_pixel_shader;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_gfx_root_signature;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_gfx_pso;
	// State object needs to be alive for the id to work
	Microsoft::WRL::ComPtr<ID3D12StateObject> m_gfx_so;
	D3D12_PROGRAM_IDENTIFIER m_program_id;
};

void CreateGraphicsResources
(
	DXContext& dx_context, const DXCompiler& dx_compiler, const DXWindow& dx_window, 
	GraphicsResources& resource, bool use_old_pso
)
{
	dx_compiler.Compile(dx_context.GetDevice(), &resource.m_vertex_shader, "VertexShader.hlsl", ShaderType::VERTEX_SHADER);
	dx_compiler.Compile(dx_context.GetDevice(), &resource.m_pixel_shader, "PixelShader.hlsl", ShaderType::PIXEL_SHADER);

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
		vertex_upload_buffer.CreateResource(dx_context, "VertexUploadBuffer");

		Vertex* data = nullptr;
		vertex_upload_buffer.m_resource->Map(0, nullptr, reinterpret_cast<void**>(&data)) >> CHK;
		memcpy(data, vertex_data, sizeof(vertex_data));
		vertex_upload_buffer.m_resource->Unmap(0, nullptr);
		dx_context.InitCommandLists();
		dx_context.Transition(D3D12_RESOURCE_STATE_COPY_SOURCE, vertex_upload_buffer);
		dx_context.Transition(D3D12_RESOURCE_STATE_COPY_DEST, resource.m_vertex_buffer);
		dx_context.GetCommandListGraphics()->CopyResource(resource.m_vertex_buffer.m_resource.Get(), vertex_upload_buffer.m_resource.Get());
		dx_context.Transition(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, resource.m_vertex_buffer);
		// Copy queue has some constraints regarding copy state and barriers
		// Compute has synchronization issue
		dx_context.ExecuteCommandListGraphics();
		dx_context.Flush(1);

		// Same rootsignature for VS and PS
		dx_context.GetDevice()->CreateRootSignature(0, resource.m_vertex_shader->GetBufferPointer(), resource.m_vertex_shader->GetBufferSize(), IID_PPV_ARGS(&resource.m_gfx_root_signature)) >> CHK;
		NAME_DX_OBJECT(resource.m_gfx_root_signature, "Graphics RootSignature");

		if (use_old_pso)
		{
			const D3D12_INPUT_LAYOUT_DESC layout_desc =
			{
				.pInputElementDescs = nullptr,
				.NumElements = 0,
			};

			const D3D12_GRAPHICS_PIPELINE_STATE_DESC gfx_pso_desc
			{
				/*root signature already embed in shader*/
				.pRootSignature = nullptr,
				.VS =
				{
					.pShaderBytecode = resource.m_vertex_shader->GetBufferPointer(),
					.BytecodeLength = resource.m_vertex_shader->GetBufferSize(),
				},
				.PS =
				{
					.pShaderBytecode = resource.m_pixel_shader->GetBufferPointer(),
					.BytecodeLength = resource.m_pixel_shader->GetBufferSize(),
				},
				.DS = nullptr,
				.HS = nullptr,
				.GS = nullptr,
				.StreamOutput =
				{
					.pSODeclaration = nullptr,
					.NumEntries = 0,
					.pBufferStrides = nullptr,
					.NumStrides = 0,
					.RasterizedStream = 0,
				},
				.BlendState =
				{
					.AlphaToCoverageEnable = false,
					.IndependentBlendEnable = false,
					.RenderTarget =
					{
						{
							.BlendEnable = false,
							.LogicOpEnable = false,
							.SrcBlend = D3D12_BLEND_ZERO,
							.DestBlend = D3D12_BLEND_ZERO,
							.BlendOp = D3D12_BLEND_OP_ADD,
							.SrcBlendAlpha = D3D12_BLEND_ZERO,
							.DestBlendAlpha = D3D12_BLEND_ZERO,
							.BlendOpAlpha = D3D12_BLEND_OP_ADD,
							.LogicOp = D3D12_LOGIC_OP_NOOP,
							.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
						},
					},
				},
				.SampleMask = 0xFFFFFFFF,
				.RasterizerState =
				{
					.FillMode = D3D12_FILL_MODE_SOLID,
					.CullMode = D3D12_CULL_MODE_NONE,
					.FrontCounterClockwise = false,
					.DepthBias = 0,
					.DepthBiasClamp = 0.0f,
					.SlopeScaledDepthBias = 0.0f,
					.DepthClipEnable = false,
					.MultisampleEnable = false,
					.AntialiasedLineEnable = false,
					.ForcedSampleCount = 0,
				},
				.DepthStencilState =
				{
					.DepthEnable = false,
					.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO,
					.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS,
					.StencilEnable = false,
					.StencilReadMask = 0,
					.StencilWriteMask = 0,
					.FrontFace =
					{
						.StencilFailOp = D3D12_STENCIL_OP_KEEP,
						.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
						.StencilPassOp = D3D12_STENCIL_OP_KEEP,
						.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS,
					},
					.BackFace =
					{
						.StencilFailOp = D3D12_STENCIL_OP_KEEP,
						.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
						.StencilPassOp = D3D12_STENCIL_OP_KEEP,
						.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS,
					},
				},
				.InputLayout = layout_desc,
				//.IBStripCutValue = nullptr,
				.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				.NumRenderTargets = 1,
				.RTVFormats =
				{
					dx_window.GetFormat(),
				},
				.DSVFormat = DXGI_FORMAT_UNKNOWN,
				.SampleDesc =
				{
					.Count = 1,
					.Quality = 0,
				},
				.NodeMask = 0,
				.CachedPSO =
				{
					.pCachedBlob = nullptr,
					.CachedBlobSizeInBytes = D3D12_PIPELINE_STATE_FLAG_NONE,
				},
				.Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
			};
			dx_context.GetDevice()->CreateGraphicsPipelineState(&gfx_pso_desc, IID_PPV_ARGS(&resource.m_gfx_pso)) >> CHK;
		}
		else
		{
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
			D3D12_SHADER_BYTECODE vs_byte_code
			{
				.pShaderBytecode = resource.m_vertex_shader->GetBufferPointer(),
				.BytecodeLength = resource.m_vertex_shader->GetBufferSize(),
			};
			vs_subobj->SetDXILLibrary(&vs_byte_code);
			vs_subobj->DefineExport(vertexshader_entrypoint.c_str(), common_entrypoint.c_str());
			CD3DX12_DXIL_LIBRARY_SUBOBJECT* ps_subobj = cstate_object_desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
			D3D12_SHADER_BYTECODE ps_byte_code
			{
				.pShaderBytecode = resource.m_pixel_shader->GetBufferPointer(),
				.BytecodeLength = resource.m_pixel_shader->GetBufferSize(),
			};
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
			global_rootsignature_subobj->SetRootSignature(resource.m_gfx_root_signature.Get());
			// TODO: CD3DX12 cause PIX to fail on CreateSO
			dx_context.GetDevice()->CreateStateObject(cstate_object_desc, IID_PPV_ARGS(&resource.m_gfx_so)) >> CHK;
			NAME_DX_OBJECT(resource.m_gfx_so, "Graphics State Object");

			Microsoft::WRL::ComPtr<ID3D12StateObjectProperties1> state_object_properties;
			resource.m_gfx_so.As(&state_object_properties) >> CHK;
			resource.m_program_id = state_object_properties->GetProgramIdentifier(graphics_wname.c_str());
		}
	}
}

void GraphicsWork
(
	DXContext& dx_context, DXWindow& dx_window, 
	GraphicsResources& resource, 
	DXTextureResource& output_resource,
	bool use_old_pso
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

		dx_context.GetCommandListGraphics()->SetGraphicsRootSignature(resource.m_gfx_root_signature.Get());
		if (use_old_pso)
		{
			dx_context.GetCommandListGraphics()->SetPipelineState(resource.m_gfx_pso.Get());
		}
		else
		{
			D3D12_SET_PROGRAM_DESC program_desc
			{
				.Type = D3D12_PROGRAM_TYPE_GENERIC_PIPELINE,
				.GenericPipeline =
				{
					.ProgramIdentifier = resource.m_program_id,
				},
			};
			dx_context.GetCommandListGraphics()->SetProgram(&program_desc);
		}
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
	ComputeResources& compute_resource,
	bool use_old_pso
)
{
	// Fill CommandList
	dx_window.BeginFrame(dx_context);

	// Set descriptor heap before root signature, order required by spec
	dx_context.GetCommandListGraphics()->SetDescriptorHeaps(1, dx_context.m_resources_descriptor_heap.m_heap.GetAddressOf());
	{
		ComputeWork(dx_context, compute_resource, dx_window, dx_window.m_buffers[g_current_buffer_index]);
		//GraphicsWork(dx_context, dx_window, gfx_resource, dx_window.m_buffers[g_current_buffer_index], use_old_pso);
	}
	dx_window.EndFrame(dx_context);
}

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

		DescriptorHeap imgui_descriptor_heap{};
		dx_context.CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, "ImGui descriptor heap", imgui_descriptor_heap);
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor = dx_context.GetCPUDescriptorHandle(imgui_descriptor_heap, 0);
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_descriptor = dx_context.GetGPUDescriptorHandle(imgui_descriptor_heap, 0);

		{
			// Setup Dear ImGui context
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
			//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

			// Setup Platform/Renderer backends
			ImGui_ImplWin32_Init(dx_window.m_handle);
			ImGui_ImplDX12_Init(dx_context.GetDevice().Get(), g_backbuffer_count, DXGI_FORMAT_R8G8B8A8_UNORM,
			imgui_descriptor_heap.m_heap.Get(),
			// You'll need to designate a descriptor from your descriptor heap for Dear ImGui to use internally for its font texture's SRV
			cpu_descriptor,
			gpu_descriptor);
		}

		{
			GraphicsResources gfx_resource{};
			bool uses_pix = dynamic_cast<PIXCapture*>(gpu_capture);
			CreateGraphicsResources(dx_context, dx_compiler, dx_window, gfx_resource, uses_pix);
			ComputeResources compute_resource{};
			CreateComputeResources(dx_context, dx_compiler, compute_resource);

			// Indirect buffers
			Microsoft::WRL::ComPtr<IDxcBlob> indirect_shader{};
			dx_compiler.Compile(dx_context.GetDevice(), &indirect_shader, "IndirectShader.hlsl", ShaderType::COMPUTE_SHADER);

			Microsoft::WRL::ComPtr<ID3D12RootSignature> indirect_root_signature{};
			// Root signature embed in the shader
			dx_context.GetDevice()->CreateRootSignature(0, indirect_shader->GetBufferPointer(), indirect_shader->GetBufferSize(), IID_PPV_ARGS(&indirect_root_signature)) >> CHK;
			NAME_DX_OBJECT(indirect_root_signature, "Indirect RootSignature");

			std::string indirect_name{ "Indirect" };
			std::wstring indirect_wname = std::to_wstring(indirect_name);

			CD3DX12_STATE_OBJECT_DESC cstate_object_desc;
			cstate_object_desc.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);

			CD3DX12_DXIL_LIBRARY_SUBOBJECT* cs_subobj = cstate_object_desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
			D3D12_SHADER_BYTECODE cs_byte_code
			{
				.pShaderBytecode = indirect_shader->GetBufferPointer(),
				.BytecodeLength = indirect_shader->GetBufferSize(),
			};
			cs_subobj->SetDXILLibrary(&cs_byte_code);
			CD3DX12_GENERIC_PROGRAM_SUBOBJECT* generic_subobj = cstate_object_desc.CreateSubobject<CD3DX12_GENERIC_PROGRAM_SUBOBJECT>();
			generic_subobj->SetProgramName(indirect_wname.c_str());
			generic_subobj->AddExport(L"main");
			// Apparently works without linking SO with RS, though RS is embedded in shader already but seem to be needed for graphics SO
			Microsoft::WRL::ComPtr<ID3D12StateObject> indirect_so{};
			dx_context.GetDevice()->CreateStateObject(cstate_object_desc, IID_PPV_ARGS(&indirect_so)) >> CHK;
			NAME_DX_OBJECT(indirect_so, "Indirect State Object");

			Microsoft::WRL::ComPtr<ID3D12StateObjectProperties1> state_object_properties;
			indirect_so.As(&state_object_properties) >> CHK;
			D3D12_PROGRAM_IDENTIFIER indirect_program_id = state_object_properties->GetProgramIdentifier(indirect_wname.c_str());
			
			//////////////////////////////////////////////////////////////////////////
			// Indirect Vertex buffers
			Microsoft::WRL::ComPtr<IDxcBlob> vertex_indirect_shader{};
			dx_compiler.Compile(dx_context.GetDevice(), &vertex_indirect_shader, "FillVertexBufferShader.hlsl", ShaderType::COMPUTE_SHADER);

			Microsoft::WRL::ComPtr<ID3D12RootSignature> vertex_indirect_root_signature{};
			// Root signature embed in the shader
			dx_context.GetDevice()->CreateRootSignature(0, vertex_indirect_shader->GetBufferPointer(), vertex_indirect_shader->GetBufferSize(), IID_PPV_ARGS(&vertex_indirect_root_signature)) >> CHK;
			NAME_DX_OBJECT(indirect_root_signature, "Indirect RootSignature");

			std::string vertex_indirect_name{ "Vertex Indirect" };
			std::wstring vertex_indirect_wname = std::to_wstring(vertex_indirect_name);

			CD3DX12_STATE_OBJECT_DESC vertex_cstate_object_desc;
			vertex_cstate_object_desc.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);

			CD3DX12_DXIL_LIBRARY_SUBOBJECT* vertex_cs_subobj = vertex_cstate_object_desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
			D3D12_SHADER_BYTECODE vertex_cs_byte_code
			{
				.pShaderBytecode = vertex_indirect_shader->GetBufferPointer(),
				.BytecodeLength = vertex_indirect_shader->GetBufferSize(),
			};
			vertex_cs_subobj->SetDXILLibrary(&vertex_cs_byte_code);
			CD3DX12_GENERIC_PROGRAM_SUBOBJECT* vertex_generic_subobj = vertex_cstate_object_desc.CreateSubobject<CD3DX12_GENERIC_PROGRAM_SUBOBJECT>();
			vertex_generic_subobj->SetProgramName(vertex_indirect_wname.c_str());
			vertex_generic_subobj->AddExport(L"main");
			// Apparently works without linking SO with RS, though RS is embedded in shader already but seem to be needed for graphics SO
			Microsoft::WRL::ComPtr<ID3D12StateObject> vertex_indirect_so{};
			dx_context.GetDevice()->CreateStateObject(vertex_cstate_object_desc, IID_PPV_ARGS(&vertex_indirect_so)) >> CHK;
			NAME_DX_OBJECT(vertex_indirect_so, "Vertex Indirect State Object");

			Microsoft::WRL::ComPtr<ID3D12StateObjectProperties1> vertex_state_object_properties;
			vertex_indirect_so.As(&vertex_state_object_properties) >> CHK;
			D3D12_PROGRAM_IDENTIFIER vertex_indirect_program_id = vertex_state_object_properties->GetProgramIdentifier(vertex_indirect_wname.c_str());
			
			D3D12_INDIRECT_ARGUMENT_DESC indirect_desc
			{
				.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW,
			};
			D3D12_COMMAND_SIGNATURE_DESC command_signature_desc
			{
				.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS),
				.NumArgumentDescs = 1,
				.pArgumentDescs = &indirect_desc,
				.NodeMask = 0,
			};
			Microsoft::WRL::ComPtr<ID3D12CommandSignature> command_signature{};
			dx_context.GetDevice()->CreateCommandSignature(&command_signature_desc, nullptr, IID_PPV_ARGS(&command_signature)) >> CHK;
			NAME_DX_OBJECT(command_signature, "Command Signature");

			while (!dx_window.ShouldClose())
			{
				// Process window message
				dx_window.Update();
			
				// (Your code process and dispatch Win32 messages)
				// Start the Dear ImGui frame
				ImGui_ImplDX12_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();
				ImGui::ShowDemoWindow(); // Show demo window! :)

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

				if (capture)
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
						//PIXScopedEvent(dx_context.GetCommandListGraphics(), 0, "My Frame");
						
						dx_context.InitCommandLists();
						FillCommandList(dx_context, dx_window, gfx_resource, compute_resource, uses_pix);
						
						{
							// Indirect debug draw
							// Create resource buffer for indirect argument
							struct IndexedInstancedDrawArgs
							{
								uint32 IndexCountPerInstance;
								uint32 InstanceCount;
								uint32 StartIndexLocation;
								int32  BaseVertexLocation;
								uint32 StartInstanceLocation;
							};
							auto size = (uint32)sizeof(IndexedInstancedDrawArgs);
							DXResource indirect_buffer{};
							indirect_buffer.SetResourceInfo(D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, size);
							indirect_buffer.CreateResource(dx_context, "Indirect Buffer");
							dx_context.m_resource_handler.RegisterResource(indirect_buffer);

							struct MyCBuffer
							{
								uint32 bindless_index;
							};

							// Indirect Work
							// Transition to UAV
							dx_context.Transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, indirect_buffer);

							D3D12_SET_PROGRAM_DESC program_desc
							{
								.Type = D3D12_PROGRAM_TYPE_GENERIC_PIPELINE,
								.GenericPipeline =
								{
									.ProgramIdentifier = indirect_program_id
								},
							};
							dx_context.GetCommandListGraphics()->SetProgram(&program_desc);

							D3D12_UNORDERED_ACCESS_VIEW_DESC UAV_desc = GetStructuredBufferUAVDesc(1, sizeof(IndexedInstancedDrawArgs));
							UAV uav = dx_context.CreateUAV(indirect_buffer, UAV_desc);

							MyCBuffer cbuffer
							{
								.bindless_index = uav.m_bindless_index
							};

							dx_context.GetCommandListGraphics()->SetComputeRootSignature(indirect_root_signature.Get());

							dx_context.GetCommandListGraphics()->SetComputeRoot32BitConstants(0, sizeof(MyCBuffer) / 4, &cbuffer, 0);
							uint32 dispatch_x = DivideRoundUp(1, 8);
							uint32 dispatch_y = DivideRoundUp(1, 8);
							dx_context.GetCommandListGraphics()->Dispatch(dispatch_x, dispatch_y, 1);
							dx_context.Transition(D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, indirect_buffer);

							// Create Vertex Buffer
							// Run shader that fills buffer

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

							DXVertexBufferResource vertex_buffer_indirect{};
							vertex_buffer_indirect.SetResourceInfo(D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, sizeof(vertex_data), sizeof(vertex_data[0]));
							vertex_buffer_indirect.CreateResource(dx_context, "VertexBuffer Indirect");
							dx_context.m_resource_handler.RegisterResource(vertex_buffer_indirect);

							struct VertexMyCBuffer
							{
								uint32 bindless_index;
							};

							dx_context.Transition(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, vertex_buffer_indirect);

							D3D12_SET_PROGRAM_DESC vertex_program_desc
							{
								.Type = D3D12_PROGRAM_TYPE_GENERIC_PIPELINE,
								.GenericPipeline =
								{
									.ProgramIdentifier = vertex_indirect_program_id
								},
							};
							dx_context.GetCommandListGraphics()->SetProgram(&vertex_program_desc);

							D3D12_UNORDERED_ACCESS_VIEW_DESC vertex_UAV_desc = GetStructuredBufferUAVDesc(1, sizeof(Vertex));
							UAV vertex_uav = dx_context.CreateUAV(vertex_buffer_indirect, vertex_UAV_desc);

							VertexMyCBuffer vertex_cbuffer
							{
								.bindless_index = vertex_uav.m_bindless_index
							};

							dx_context.GetCommandListGraphics()->SetComputeRootSignature(vertex_indirect_root_signature.Get());

							dx_context.GetCommandListGraphics()->SetComputeRoot32BitConstants(0, sizeof(VertexMyCBuffer) / 4, &vertex_cbuffer, 0);
							uint32 dispatch_x1 = DivideRoundUp(1, 8);
							uint32 dispatch_y1 = DivideRoundUp(1, 8);
							dx_context.GetCommandListGraphics()->Dispatch(dispatch_x1, dispatch_y1, 1);
							dx_context.Transition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, vertex_buffer_indirect);
							
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

							D3D12_SHADER_RESOURCE_VIEW_DESC desc = GetStructuredBufferSRVDesc(vertex_buffer_indirect.m_count, vertex_buffer_indirect.m_stride);
							SRV srv = dx_context.CreateSRV(vertex_buffer_indirect, desc);

							dx_context.GetCommandListGraphics()->SetGraphicsRootSignature(gfx_resource.m_gfx_root_signature.Get());
							
							if (uses_pix)
							{
								dx_context.GetCommandListGraphics()->SetPipelineState(gfx_resource.m_gfx_pso.Get());
							}
							else
							{
								D3D12_SET_PROGRAM_DESC program_desc
								{
									.Type = D3D12_PROGRAM_TYPE_GENERIC_PIPELINE,
									.GenericPipeline =
									{
										.ProgramIdentifier = gfx_resource.m_program_id,
									},
								};
								dx_context.GetCommandListGraphics()->SetProgram(&program_desc);
							}
							uint32 bindless_index = srv.m_bindless_index;
							dx_context.GetCommandListGraphics()->SetGraphicsRoot32BitConstants(0, 1, &bindless_index, 0);
							dx_context.GetCommandListGraphics()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
							// transition

							dx_context.Transition(D3D12_RESOURCE_STATE_RENDER_TARGET, dx_window.m_buffers[g_current_buffer_index]);
							D3D12_RENDER_TARGET_VIEW_DESC rtv_desc
							{
								.Format = dx_window.m_buffers[g_current_buffer_index].m_format,
								.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
								.Texture2D =
								{
									.MipSlice = 0,
									.PlaneSlice = 0,
								},
							};
							ID3D12Resource* d3d12_ouput_resource = dx_window.m_buffers[g_current_buffer_index].m_resource.Get();
							dx_context.OMSetRenderTargets(1, &d3d12_ouput_resource, &rtv_desc, nullptr, nullptr);
							dx_context.GetCommandListGraphics()->ExecuteIndirect(command_signature.Get() , 1, indirect_buffer.m_resource.Get(), 0, nullptr, 0);
						}

						// Rendering
						// (Your code clears your framebuffer, renders your other stuff etc.)
						dx_context.Transition(D3D12_RESOURCE_STATE_RENDER_TARGET, dx_window.m_buffers[g_current_buffer_index]);
						ImGui::Render();
						dx_context.GetCommandListGraphics()->SetDescriptorHeaps(1, imgui_descriptor_heap.m_heap.GetAddressOf());
						ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dx_context.GetCommandListGraphics().Get());
						// (Your code calls ExecuteCommandLists, swapchain's Present(), etc.)
						dx_context.Transition(D3D12_RESOURCE_STATE_PRESENT, dx_window.m_buffers[g_current_buffer_index]);


						dx_context.ExecuteCommandListGraphics();
						dx_window.Present(dx_context);
					}
				}
				if (capture)
				{
					gpu_capture->EndCapture();
					gpu_capture->OpenCapture();
				}

			}
			dx_context.Flush(dx_window.GetBackBufferCount());
		}

		{
			ImGui_ImplDX12_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
		}
	}
}
#pragma endregion

#pragma region COMPUTE
void CreateComputeResources(DXContext& dx_context, const DXCompiler& dx_compiler, ComputeResources& resource)
{
	dx_compiler.Compile(dx_context.GetDevice(), &resource.m_compute_shader, "ComputeShader.hlsl", ShaderType::COMPUTE_SHADER);

	// Root signature embed in the shader
	dx_context.GetDevice()->CreateRootSignature(0, resource.m_compute_shader->GetBufferPointer(), resource.m_compute_shader->GetBufferSize(), IID_PPV_ARGS(&resource.m_compute_root_signature)) >> CHK;
	NAME_DX_OBJECT(resource.m_compute_root_signature, "Compute RootSignature");

	std::string compute_name{ "Compute" };
	std::wstring compute_wname = std::to_wstring(compute_name);

	CD3DX12_STATE_OBJECT_DESC cstate_object_desc;
	cstate_object_desc.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);

	CD3DX12_DXIL_LIBRARY_SUBOBJECT* cs_subobj = cstate_object_desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	D3D12_SHADER_BYTECODE cs_byte_code
	{
		.pShaderBytecode = resource.m_compute_shader->GetBufferPointer(), 
		.BytecodeLength = resource.m_compute_shader->GetBufferSize(), 
	};
	cs_subobj->SetDXILLibrary(&cs_byte_code);
	CD3DX12_GENERIC_PROGRAM_SUBOBJECT* generic_subobj = cstate_object_desc.CreateSubobject<CD3DX12_GENERIC_PROGRAM_SUBOBJECT>();
	generic_subobj->SetProgramName(compute_wname.c_str());
	generic_subobj->AddExport(L"main");
	// Apparently works without linking SO with RS, though RS is embedded in shader already but seem to be needed for graphics SO
	dx_context.GetDevice()->CreateStateObject(cstate_object_desc, IID_PPV_ARGS(&resource.m_compute_so)) >> CHK;
	NAME_DX_OBJECT(resource.m_compute_so, "Compute State Object");

	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties1> state_object_properties;
	resource.m_compute_so.As(&state_object_properties) >> CHK;
	resource.m_program_id = state_object_properties->GetProgramIdentifier(compute_wname.c_str());
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
			.ProgramIdentifier = compute_resource.m_program_id
		},
	};
	dx_context.GetCommandListGraphics()->SetProgram(&program_desc);

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAV_desc = GetTexture2DUAVDesc(gpu_resource.m_format);
	UAV uav = dx_context.CreateUAV(gpu_resource, UAV_desc);
	
	auto current_time = std::chrono::high_resolution_clock::now();
	auto diff_seconds = (float32)std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
	diff_seconds /= 1000;

	MyCBuffer cbuffer
	{
		.resolution = {(uint32)gpu_resource.m_resource_desc.Width, (uint32)gpu_resource.m_resource_desc.Height},
		.iTime = diff_seconds,
		.bindless_index = uav.m_bindless_index
	};
	
	dx_context.GetCommandListGraphics()->SetComputeRootSignature(compute_resource.m_compute_root_signature.Get());
	
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

	D3D12_PROGRAM_IDENTIFIER m_program_id;

	Microsoft::WRL::ComPtr<IDxcBlob> m_workgraph_shader;
	Microsoft::WRL::ComPtr<ID3D12StateObject> m_workgraph_so;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_workgraph_root_signature;
};

void CreateWorkGraphResource
 (
	 DXContext& dx_context, DXCompiler& dx_compiler, 
	 WorkGraphResources& resource,
	 bool is_pix_running
 )
{
	dx_compiler.Compile(dx_context.GetDevice(), &resource.m_workgraph_shader, "WorkGraphShader.hlsl", ShaderType::LIB_SHADER);

	D3D12_SHADER_BYTECODE byte_code
	{
		.pShaderBytecode = resource.m_workgraph_shader->GetBufferPointer(),
		.BytecodeLength = resource.m_workgraph_shader->GetBufferSize()
	};

	dx_context.GetDevice()->CreateRootSignature(0, byte_code.pShaderBytecode, byte_code.BytecodeLength, IID_PPV_ARGS(&resource.m_workgraph_root_signature)) >> CHK;
	NAME_DX_OBJECT(resource.m_workgraph_root_signature, "Global RootSignature");

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
	rootsignature_subobj->SetRootSignature(resource.m_workgraph_root_signature.Get());
	dx_context.GetDevice()->CreateStateObject(cstate_object_desc, IID_PPV_ARGS(&resource.m_workgraph_so)) >> CHK;
	NAME_DX_OBJECT(resource.m_workgraph_so, "Workgraph State Object");

	// These are not subtypes but you can QueryInterface as how ComPtr works apparently
	// To figure out from which to what you can QueryInterface, look up the docs they said ..
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties1> state_object_properties;
	resource.m_workgraph_so.As(&state_object_properties) >> CHK;
	resource.m_program_id = state_object_properties->GetProgramIdentifier(work_graph_wname.c_str());

	Microsoft::WRL::ComPtr<ID3D12WorkGraphProperties> workgraph_properties;
	resource.m_workgraph_so.As(&workgraph_properties) >> CHK;
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
			.ProgramIdentifier = resource.m_program_id,
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
	dx_context.GetCommandListGraphics()->SetComputeRootSignature(resource.m_workgraph_root_signature.Get());
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

#pragma region WINDOW
class Application
{
public:
	void Init();
	void Close();
	void Run();
private:
};

LRESULT testCallback(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(handle, msg, wParam, lParam);
}
void RunTest()
{
	WindowDesc window_desc
	{
		.m_callback = testCallback,
		.m_window_name = "TEST",
		.m_width = 500,
		.m_height = 500,
		.m_origin_x = 200,
		.m_origin_y = 200,
	};
	CreateWindowNew(window_desc);
	while (true)
	{
		;
	}
}
#pragma endregion
int main()
{
	start_time = std::chrono::high_resolution_clock::now();

	MemoryTrack();

	DXReportContext dx_report_context{};
	{
		// TODO PIX / renderdoc markers
		GPUCapture * gpu_capture = nullptr;
		//GPUCapture* gpu_capture = new PIXCapture();
		// RenderDoc doesnt support WorkGraph and newer interfaces
		//GPUCapture* gpu_capture = new RenderDocCapture();

		DXContext dx_context{};
		
		dx_report_context.SetDevice(dx_context.GetDevice(), dx_context.m_adapter);
		DXCompiler dx_compiler("shaders");
		//gpu_capture->StartCapture();
		do
		{
			//RunWorkGraph(dx_context, dx_compiler, dynamic_cast<PIXCapture*>(gpu_capture) != nullptr);
		}while (false);
		//gpu_capture->EndCapture();
		//gpu_capture->OpenCapture();

		RunWindowLoop(dx_context, dx_compiler, gpu_capture);
		//gpu_capture->StartCapture();
		//RunComputeWork(dx_context);
		//gpu_capture->EndCapture();
		//gpu_capture->OpenCapture();
		//RunTest();

		delete gpu_capture;
	}
	
	return 0;
}