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


struct Resources
{
	// Compute
	DXResource m_gpu_resource;
	DXResource m_cpu_resource;
	Microsoft::WRL::ComPtr<IDxcBlob> m_compute_shader;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_compute_pso;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_compute_root_signature;
	uint32 m_group_size;
	uint32 m_dispatch_count;

	// Graphics
	Microsoft::WRL::ComPtr<IDxcBlob> m_vertex_shader;
	Microsoft::WRL::ComPtr<IDxcBlob> m_pixel_shader;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_gfx_pso;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_gfx_root_signature;
};

void CreateComputeResources(DXContext& dx_context, const DXCompiler& dx_compiler, Resources& resource)
{
	resource.m_group_size = 1024;
	resource.m_dispatch_count = 1;

	dx_compiler.Compile(dx_context.GetDevice(), &resource.m_compute_shader, "ComputeShader.hlsl", ShaderType::COMPUTE_SHADER);

	uint32 size = resource.m_group_size * resource.m_dispatch_count * sizeof(float32) * 4;
	resource.m_gpu_resource.SetResourceInfo(D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, size);
	resource.m_gpu_resource.CreateResource(dx_context, "GPU resource");

	resource.m_cpu_resource.SetResourceInfo(D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_FLAG_NONE, size);
	resource.m_cpu_resource.m_resource_state = D3D12_RESOURCE_STATE_COPY_DEST;
	resource.m_cpu_resource.CreateResource(dx_context, "CPU resource");
	
	// Root signature embed in the shader already
	dx_context.GetDevice()->CreateRootSignature(0, resource.m_compute_shader->GetBufferPointer(), resource.m_compute_shader->GetBufferSize(), IID_PPV_ARGS(&resource.m_compute_root_signature)) >> CHK;

	D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc =
	{
		.pRootSignature = /*resource.m_compute_root_signature.Get()*/nullptr,
		.CS = 
		{
			.pShaderBytecode = resource.m_compute_shader->GetBufferPointer(),
			.BytecodeLength = resource.m_compute_shader->GetBufferSize(),
		},
	};
	dx_context.GetDevice()->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&resource.m_compute_pso)) >> CHK;
}

void CreateGraphicsResources
(
	DXContext& dx_context, const DXCompiler& dx_compiler, const DXWindow& dx_window, 
	Resources& resource, 
	DXResource& vertex_buffer, D3D12_VERTEX_BUFFER_VIEW& vertex_buffer_view, 
	uint32& vertex_count
)
{
	dx_compiler.Compile(dx_context.GetDevice(), &resource.m_vertex_shader, "VertexShader.hlsl", ShaderType::VERTEX_SHADER);
	dx_compiler.Compile(dx_context.GetDevice(), &resource.m_pixel_shader, "PixelShader.hlsl", ShaderType::PIXEL_SHADER);

	D3D12_DESCRIPTOR_HEAP_DESC desc_heap_desc =
	{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
	};
	{
		struct Vertex
		{
			float2 position;
			float3 color;
		};

		const Vertex vertex_data[] =
		{
			{ {+0.0f, +0.25f}, {1.0f, 0.0f, 0.0f} },
			{ {+0.25f, -0.25f}, {0.0f, 1.0f, 0.0f} },
			{ {-0.25f, -0.25f}, {0.0f, 0.0f, 1.0f} },
		};

		vertex_count = COUNT(vertex_data);

		vertex_buffer.SetResourceInfo(D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE, sizeof(vertex_data));
		vertex_buffer.CreateResource(dx_context, "VertexBuffer");

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
		dx_context.Transition(D3D12_RESOURCE_STATE_COPY_DEST, vertex_buffer);
		dx_context.GetCommandListGraphics()->CopyResource(vertex_buffer.m_resource.Get(), vertex_upload_buffer.m_resource.Get());
		dx_context.Transition(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertex_buffer);
		// Copy queue has some constraints regarding copy state and barriers
		// Compute has synchronization issue
		dx_context.ExecuteCommandListGraphics();
		dx_context.Flush(1);

		vertex_buffer_view =
		{
			.BufferLocation = vertex_buffer.m_resource->GetGPUVirtualAddress(),
			.SizeInBytes = sizeof(vertex_data),
			.StrideInBytes = sizeof(vertex_data[0]),
		};

		const D3D12_INPUT_ELEMENT_DESC element_descs[] =
		{
			{
				.SemanticName = "Position",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT, // Auto from previous element
				.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0,
			},
			{
				.SemanticName = "Color",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT, // Auto from previous element
				.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0,
			},
		};
		const D3D12_INPUT_LAYOUT_DESC layout_desc =
		{
			.pInputElementDescs = element_descs,
			.NumElements = COUNT(element_descs),
		};

		dx_context.GetDevice()->CreateRootSignature(0, resource.m_vertex_shader->GetBufferPointer(), resource.m_vertex_shader->GetBufferSize(), IID_PPV_ARGS(&resource.m_gfx_root_signature)) >> CHK;

		const D3D12_GRAPHICS_PIPELINE_STATE_DESC gfx_pso_desc
		{
			.pRootSignature = /*resource.m_gfx_root_signature.Get()*/nullptr /*root signature already embed in shader*/,
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
}

void ComputeWork
(
	DXContext& dx_context, Resources& resource
)
{
	// Compute Work
	// Transition to UAV
	{
		D3D12_RESOURCE_STATES new_resource_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		D3D12_RESOURCE_BARRIER barriers[1]{};
		barriers[0] =
		{
			.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Transition =
			{
				.pResource = resource.m_gpu_resource.m_resource.Get(),
				.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				.StateBefore = resource.m_gpu_resource.m_resource_state,
				.StateAfter = new_resource_state,
			},
		};
		dx_context.GetCommandListGraphics()->ResourceBarrier(COUNT(barriers), &barriers[0]);
		resource.m_gpu_resource.m_resource_state = new_resource_state;
	}
	dx_context.GetCommandListGraphics()->SetComputeRootSignature(resource.m_compute_root_signature.Get());
	dx_context.GetCommandListGraphics()->SetPipelineState(resource.m_compute_pso.Get());
	dx_context.GetCommandListGraphics()->SetComputeRootUnorderedAccessView(0, resource.m_gpu_resource.m_resource->GetGPUVirtualAddress());
	dx_context.GetCommandListGraphics()->Dispatch(resource.m_dispatch_count, 1, 1);
	// Transition to Copy Src
	{
		D3D12_RESOURCE_STATES new_resource_state = D3D12_RESOURCE_STATE_COPY_SOURCE;
		D3D12_RESOURCE_BARRIER barriers[1]{};
		barriers[0] =
		{
			.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Transition =
			{
				.pResource = resource.m_gpu_resource.m_resource.Get(),
				.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				.StateBefore = resource.m_gpu_resource.m_resource_state,
				.StateAfter = new_resource_state,
			},
		};
		dx_context.GetCommandListGraphics()->ResourceBarrier(COUNT(barriers), &barriers[0]);
		resource.m_gpu_resource.m_resource_state = new_resource_state;
	}
	dx_context.GetCommandListGraphics()->CopyResource(resource.m_cpu_resource.m_resource.Get(), resource.m_gpu_resource.m_resource.Get());
}

#include "d3dx12.h"

void GraphicsWork
(
	DXContext& dx_context, DXWindow& dx_window, Resources& resource, 	
	const D3D12_VERTEX_BUFFER_VIEW& vertex_buffer_view, uint32 vertex_count
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
		dx_context.GetCommandListGraphics()->SetPipelineState(resource.m_gfx_pso.Get());
		dx_context.GetCommandListGraphics()->SetGraphicsRootSignature(resource.m_gfx_root_signature.Get());
		dx_context.GetCommandListGraphics()->IASetVertexBuffers(0, 1, &vertex_buffer_view);
		dx_context.GetCommandListGraphics()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dx_context.GetCommandListGraphics()->DrawInstanced(vertex_count, 1, 0, 0);
	}
}

void FillCommandList
(
	DXContext& dx_context, 
	DXWindow& dx_window, Resources& resource, 
	const D3D12_VERTEX_BUFFER_VIEW& vertex_buffer_view, uint32 vertex_count
)
{
	// Fill CommandList
	dx_window.BeginFrame(dx_context);
	ComputeWork(dx_context, resource);
	GraphicsWork(dx_context, dx_window, resource, vertex_buffer_view, vertex_count);
	dx_window.EndFrame(dx_context);
}
void RunTest();

void RunWorkGraph(DXContext& dx_context, DXCompiler& dx_compiler)
{
	dx_context.InitCommandLists();
	{
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmd_list = dx_context.GetCommandListGraphics();
		dx_context.InitCommandLists();
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> cmd_list10{};
		cmd_list.As(&cmd_list10) >> CHK;
		Microsoft::WRL::ComPtr<ID3D12Device> device = dx_context.GetDevice();
		Microsoft::WRL::ComPtr<ID3D12Device5> device5{};
		device.As(&device5) >> CHK;

		Microsoft::WRL::ComPtr<IDxcBlob> outShaderBlob{};
		dx_compiler.Compile(dx_context.GetDevice(), &outShaderBlob, "WorkGraphShader.hlsl", ShaderType::LIB_SHADER);

		D3D12_SHADER_BYTECODE byte_code
		{
			.pShaderBytecode = outShaderBlob->GetBufferPointer(),
			.BytecodeLength = outShaderBlob->GetBufferSize()
		};

		Microsoft::WRL::ComPtr<ID3D12Device14> device14{};
		device.As(&device14) >> CHK;

		CD3DX12_SHADER_BYTECODE libCode(byte_code);
		Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature{};
		device14->CreateRootSignature(0, libCode.pShaderBytecode, libCode.BytecodeLength, /*L"globalRS",*/ IID_PPV_ARGS(&root_signature)) >> CHK;
		NAME_DX_OBJECT(root_signature, "Global RootSignature");

		Microsoft::WRL::ComPtr<ID3D12StateObject> state_object{};

		D3D12_DXIL_LIBRARY_DESC sub_object_library =
		{
			.DXILLibrary = byte_code,
			.NumExports = 0,
			.pExports = nullptr,
		};
		std::string work_graph_name{ "WorkGraph" };
		std::wstring work_graph_wname = std::to_wstring(work_graph_name);
		D3D12_WORK_GRAPH_DESC sub_object_work_graph
		{
			.ProgramName = work_graph_wname.c_str(),
			.Flags = D3D12_WORK_GRAPH_FLAG_INCLUDE_ALL_AVAILABLE_NODES,
			.NumEntrypoints = 0,
			.pEntrypoints = nullptr,
			.NumExplicitlyDefinedNodes = 0,
			.pExplicitlyDefinedNodes = nullptr,
		};

		D3D12_GLOBAL_ROOT_SIGNATURE sub_object_global_rootsignature
		{
			.pGlobalRootSignature = root_signature.Get(),
		};

		std::vector<D3D12_STATE_SUBOBJECT> state_subobjects =
		{
			{
				.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
				.pDesc = &sub_object_library,
			},
			{
				.Type = D3D12_STATE_SUBOBJECT_TYPE_WORK_GRAPH,
				.pDesc = &sub_object_work_graph,
			},
			{
				.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,
				.pDesc = &sub_object_global_rootsignature,
			}, // Even though global rootsignature is embed in the shader, still needs to mark as subobject to the stateobject
		};

		D3D12_STATE_OBJECT_DESC state_object_desc =
		{
			.Type = D3D12_STATE_OBJECT_TYPE_EXECUTABLE,
			.NumSubobjects = (uint32)state_subobjects.size(),
			.pSubobjects = state_subobjects.data()
		};

		device14->CreateStateObject(&state_object_desc, IID_PPV_ARGS(&state_object)) >> CHK;
		NAME_DX_OBJECT(state_object, "State Object");

		// Are these subtypes?
		Microsoft::WRL::ComPtr<ID3D12StateObjectProperties1> spWGProp1;
		state_object.As(&spWGProp1) >> CHK;
		D3D12_PROGRAM_IDENTIFIER program_identifier{};
		program_identifier = spWGProp1->GetProgramIdentifier(work_graph_wname.c_str());

		Microsoft::WRL::ComPtr<ID3D12WorkGraphProperties> spWGProps;
		state_object.As(&spWGProps) >> CHK;
		UINT WorkGraphIndex = spWGProps->GetWorkGraphIndex(work_graph_wname.c_str());
		D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS MemReqs{};
		spWGProps->GetWorkGraphMemoryRequirements(WorkGraphIndex, &MemReqs);
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE BackingMemory{};
		BackingMemory.SizeInBytes = MemReqs.MaxSizeInBytes;

		DXResource scratch_resource{};
		scratch_resource.SetResourceInfo(D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE, MemReqs.MaxSizeInBytes);
		scratch_resource.CreateResource(dx_context, "Scratch Memory");

		BackingMemory.StartAddress = scratch_resource.m_resource->GetGPUVirtualAddress();

		DXResource buffer{};
		buffer.SetResourceInfo(D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 16777216 * sizeof(uint32));
		buffer.CreateResource(dx_context, "Buffer UAV resource");

		D3D12_SET_PROGRAM_DESC program_desc =
		{
			.Type = D3D12_PROGRAM_TYPE_WORK_GRAPH,
			.WorkGraph =
			{
				.ProgramIdentifier = program_identifier,
				.Flags = D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE,
				.BackingMemory = BackingMemory,
				.NodeLocalRootArgumentsTable = 0,
			},
		};

		struct Record
		{
			UINT gridSize; // : SV_DispatchGrid;
			UINT recordIndex;
		};
		std::vector<Record> records{};
		records.push_back(Record{ 1, 0 });
		records.push_back(Record{ 2, 1 });
		records.push_back(Record{ 3, 2 });
		records.push_back(Record{ 4, 3 });
		D3D12_DISPATCH_GRAPH_DESC wg_desc =
		{
			.Mode = D3D12_DISPATCH_MODE_NODE_CPU_INPUT,
			.NodeCPUInput =
			{
				.EntrypointIndex = 0,
				.NumRecords = (uint32)records.size(),
				.pRecords = records.data(),
				.RecordStrideInBytes = sizeof(Record)
			},
		};
		cmd_list10->SetComputeRootSignature(root_signature.Get());
		cmd_list10->SetComputeRootUnorderedAccessView(0, buffer.m_resource->GetGPUVirtualAddress());
		cmd_list10->SetProgram(&program_desc);
		cmd_list10->DispatchGraph(&wg_desc);

		dx_context.Transition(D3D12_RESOURCE_STATE_COPY_SOURCE, buffer);

		DXResource readback_resource{};
		readback_resource.SetResourceInfo(D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_FLAG_NONE, buffer.m_resource_desc.Width);
		readback_resource.m_resource_state = D3D12_RESOURCE_STATE_COPY_DEST;
		readback_resource.CreateResource(dx_context, "Readback resource");

		dx_context.GetCommandListGraphics()->CopyResource(readback_resource.m_resource.Get(), buffer.m_resource.Get());
	
		dx_context.ExecuteCommandListGraphics();
		dx_context.Flush(1);

		uint32* data = nullptr;
		const D3D12_RANGE range = { 0, 20/*readback_resource.m_resource_desc.Width*/ };
		readback_resource.m_resource->Map(0, &range, (void**)&data);
		for (uint32 i = 0; i < range.End - range.Begin; ++i)
		{
			LogTrace("uav workgraph[{}] = {}\n", i, data[i]);
		}
		readback_resource.m_resource->Unmap(0, nullptr);
	}
	LogTrace("Workgraph Dispatched");
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
		{
			Resources resource{};
			CreateComputeResources(dx_context, dx_compiler, resource);
			DXResource vertex_buffer{};
			D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view{};
			uint32 vertex_count{};
			CreateGraphicsResources(dx_context, dx_compiler, dx_window, resource, vertex_buffer, vertex_buffer_view, vertex_count);

			while (!dx_window.ShouldClose())
			{
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
						dx_window.Resize(dx_context);
					}

					dx_context.InitCommandLists();
					FillCommandList(dx_context, dx_window, resource, vertex_buffer_view, vertex_count);
					dx_context.ExecuteCommandListGraphics();
					dx_window.Present(dx_context);

					float32* data = nullptr;
					const D3D12_RANGE range = { 0, resource.m_gpu_resource.m_resource_desc.Width };
					resource.m_cpu_resource.m_resource->Map(0, &range, (void**)&data);
					static bool has_display = false;
					if (!has_display)
					{
						for (uint32 i = 0; i < resource.m_cpu_resource.m_resource_desc.Width / sizeof(float32) / 4; i++)
						{
							LogTrace("uav[{}] = {}, {}, {}, {}\n", i, data[i * 4 + 0], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]);
						}
						has_display = true;
					}
					resource.m_cpu_resource.m_resource->Unmap(0, nullptr);
				}
				if (capture)
				{
					gpu_capture->EndCapture();
					gpu_capture->OpenCapture();
				}

			}
			dx_context.Flush(dx_window.GetBackBufferCount());
		}
	}
}

int main()
{
	MemoryTrack();

	DXReportContext dx_report_context{};
	{
		// TODO PIX / renderdoc markers
		//GPUCapture * gpu_capture = nullptr;
		GPUCapture* gpu_capture = new PIXCapture();
		//GPUCapture* gpu_capture = new RenderDocCapture();
		
		DXContext dx_context{};
		dx_report_context.SetDevice(dx_context.GetDevice());
		DXCompiler dx_compiler("shaders");
		//gpu_capture->StartCapture();
		do
		{
			RunWorkGraph(dx_context, dx_compiler);
		}while (false);
		//gpu_capture->EndCapture();
		//gpu_capture->OpenCapture();

		RunWindowLoop(dx_context, dx_compiler, gpu_capture);
		//RunTest();

		delete gpu_capture;
	}
	
	return 0;
}

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