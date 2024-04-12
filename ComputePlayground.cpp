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
	UAV m_uav;
	ComPtr<IDxcBlob> m_compute_shader;
	ComPtr<ID3D12PipelineState> m_compute_pso;
	ComPtr<ID3D12RootSignature> m_compute_root_signature;
	uint32 m_group_size;
	uint32 m_dispatch_count;

	// Graphics
	ComPtr<IDxcBlob> m_vertex_shader;
	ComPtr<IDxcBlob> m_pixel_shader;
	ComPtr<ID3D12PipelineState> m_gfx_pso;
	ComPtr<ID3D12RootSignature> m_gfx_root_signature;
};

void CreateComputeResources(const DXContext& dx_context, const DXCompiler& dx_compiler, Resources& resource)
{
	resource.m_group_size = 1024;
	resource.m_dispatch_count = 1;

	dx_compiler.Compile(dx_context.GetDevice(), &resource.m_compute_shader, "ComputeShader.hlsl", ShaderType::COMPUTE_SHADER);

	resource.m_uav.m_desc.Width = resource.m_group_size * resource.m_dispatch_count * sizeof(float32) * 4;
	resource.m_uav.m_readback_desc.Width = resource.m_uav.m_desc.Width;
	// Buffers have to start in COMMON_STATE, so we need to transition to UAV when recording
	D3D12_RESOURCE_STATES initial_gpu_resource_state = D3D12_RESOURCE_STATE_COMMON;
	dx_context.GetDevice()->CreateCommittedResource(&resource.m_uav.m_heap_properties, D3D12_HEAP_FLAG_NONE, &resource.m_uav.m_desc, initial_gpu_resource_state, nullptr, IID_PPV_ARGS(&resource.m_uav.m_gpu_resource)) >> CHK;
	resource.m_uav.m_gpu_resource_state = initial_gpu_resource_state;
	D3D12_RESOURCE_STATES initial_readback_resource_state = D3D12_RESOURCE_STATE_COPY_DEST;
	dx_context.GetDevice()->CreateCommittedResource(&resource.m_uav.m_readback_heap_properties, D3D12_HEAP_FLAG_NONE, &resource.m_uav.m_readback_desc, initial_readback_resource_state, nullptr, IID_PPV_ARGS(&resource.m_uav.m_read_back_resource)) >> CHK;
	resource.m_uav.m_readback_resource_state = initial_gpu_resource_state;
	// Root signature embed in the shader already
	dx_context.GetDevice()->CreateRootSignature(0, resource.m_compute_shader->GetBufferPointer(), resource.m_compute_shader->GetBufferSize(), IID_PPV_ARGS(&resource.m_compute_root_signature)) >> CHK;

	D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc =
	{
		.pRootSignature = resource.m_compute_root_signature.Get(),
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

		const D3D12_RESOURCE_DESC vertex_desc =
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, // 64 * 1024 bytes = 64kB
			.Width = sizeof(vertex_data),
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc =
			{
				.Count = 1,
				.Quality = 0,
			},
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR, // In order, no swizzle
			.Flags = D3D12_RESOURCE_FLAG_NONE,
		};
		const D3D12_HEAP_PROPERTIES heap_properties =
		{
			.Type = D3D12_HEAP_TYPE_DEFAULT,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1,
		};
		D3D12_RESOURCE_STATES initial_vertex_buffer_resource_state = D3D12_RESOURCE_STATE_COMMON;
		dx_context.GetDevice()->CreateCommittedResource
		(
			&heap_properties, D3D12_HEAP_FLAG_NONE, &vertex_desc,
			initial_vertex_buffer_resource_state, nullptr,
			IID_PPV_ARGS(&vertex_buffer.m_resource)
		) >> CHK;
		NAME_DX_OBJECT(vertex_buffer.m_resource, "VertexBuffer");
		vertex_buffer.m_resource_state = initial_vertex_buffer_resource_state;

		const D3D12_HEAP_PROPERTIES heap_properties_upload =
		{
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1,
		};
		DXResource vertex_upload_buffer{};
		D3D12_RESOURCE_STATES initial_vertex_upload_buffer_resource_state = D3D12_RESOURCE_STATE_GENERIC_READ;
		dx_context.GetDevice()->CreateCommittedResource
		(
			&heap_properties_upload, D3D12_HEAP_FLAG_NONE, &vertex_desc,
			initial_vertex_upload_buffer_resource_state, nullptr,
			IID_PPV_ARGS(&vertex_upload_buffer.m_resource)
		) >> CHK;
		NAME_DX_OBJECT(vertex_upload_buffer.m_resource, "VertexUploadBuffer");
		vertex_upload_buffer.m_resource_state = initial_vertex_upload_buffer_resource_state;

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
			.pRootSignature = resource.m_gfx_root_signature.Get(),
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
				.pResource = resource.m_uav.m_gpu_resource.Get(),
				.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				.StateBefore = resource.m_uav.m_gpu_resource_state,
				.StateAfter = new_resource_state,
			},
		};
		dx_context.GetCommandListGraphics()->ResourceBarrier(COUNT(barriers), &barriers[0]);
		resource.m_uav.m_gpu_resource_state = new_resource_state;
	}
	dx_context.GetCommandListGraphics()->SetComputeRootSignature(resource.m_compute_root_signature.Get());
	dx_context.GetCommandListGraphics()->SetPipelineState(resource.m_compute_pso.Get());
	dx_context.GetCommandListGraphics()->SetComputeRootUnorderedAccessView(0, resource.m_uav.m_gpu_resource->GetGPUVirtualAddress());
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
				.pResource = resource.m_uav.m_gpu_resource.Get(),
				.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				.StateBefore = resource.m_uav.m_gpu_resource_state,
				.StateAfter = new_resource_state,
			},
		};
		dx_context.GetCommandListGraphics()->ResourceBarrier(COUNT(barriers), &barriers[0]);
		resource.m_uav.m_gpu_resource_state = new_resource_state;
	}
	dx_context.GetCommandListGraphics()->CopyResource(resource.m_uav.m_read_back_resource.Get(), resource.m_uav.m_gpu_resource.Get());
}

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

int main()
{
	MemoryTrackStart();

	DXReportContext dx_report_context{};
	{
		// TODO PIX / renderdoc markers
		GPUCapture* gpu_capture = new PIXCapture();
		//GPUCapture* gpu_capture = new RenderDocCapture();
		DXContext dx_context{};
		dx_report_context.SetDevice(dx_context.GetDevice());
		DXCompiler dx_compiler("shaders");
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
						dx_window.ToggleWindowMode();
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
						dx_window.Present();

						float32* data = nullptr;
						const D3D12_RANGE range = { 0, resource.m_uav.m_readback_desc.Width };
						resource.m_uav.m_read_back_resource->Map(0, &range, (void**)&data);
						static bool has_display = false;
						if (!has_display)
						{
							for (uint32 i = 0; i < resource.m_uav.m_readback_desc.Width / sizeof(float32) / 4; i++)
							{
								LogTrace("uav[{}] = {}, {}, {}, {}\n", i, data[i * 4 + 0], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]);
							}
							has_display = true;
						}
						resource.m_uav.m_read_back_resource->Unmap(0, nullptr);
					}
					if (capture)
					{
						gpu_capture->EndCapture();
						gpu_capture->OpenCapture();
						//state.m_capture = !state.m_capture;
					}

				}
				dx_context.Flush(dx_window.GetBackBufferCount());
			}
		}
		delete gpu_capture;
	}
	
	return 0;
}