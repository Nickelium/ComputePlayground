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
	UAV m_uav;
	ComPtr<IDxcBlob> m_compute_shader;
	ComPtr<ID3D12PipelineState> m_compute_pso;
	ComPtr<ID3D12RootSignature> m_compute_root_signature;
	uint32 m_group_size;
	uint32 m_dispatch_count;

	// TODO add inputlayout
	ComPtr<IDxcBlob> m_vertex_shader;
	ComPtr<IDxcBlob> m_pixel_shader;
	ComPtr<ID3D12PipelineState> m_gfx_pso;
	ComPtr<ID3D12RootSignature> m_gfx_root_signature;
};

Resources CreateResources(const DXContext& dx_context, const DXCompiler& dx_compiler)
{
	Resources resources{};
	resources.m_group_size = 1024;
	resources.m_dispatch_count = 1;

	dx_compiler.Compile(dx_context.GetDevice(), &resources.m_compute_shader, "ComputeShader.hlsl", ShaderType::COMPUTE_SHADER);
	dx_compiler.Compile(dx_context.GetDevice(), &resources.m_vertex_shader, "VertexShader.hlsl", ShaderType::VERTEX_SHADER);
	dx_compiler.Compile(dx_context.GetDevice(), &resources.m_pixel_shader, "PixelShader.hlsl", ShaderType::PIXEL_SHADER);

	resources.m_uav.m_desc.Width = resources.m_group_size * resources.m_dispatch_count * sizeof(float32) * 4;
	resources.m_uav.m_readback_desc.Width = resources.m_uav.m_desc.Width;
	// TODO do we need transition to proper state when using?
	dx_context.GetDevice()->CreateCommittedResource(&resources.m_uav.m_heap_properties, D3D12_HEAP_FLAG_NONE, &resources.m_uav.m_desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resources.m_uav.m_gpu_resource)) >> CHK;
	dx_context.GetDevice()->CreateCommittedResource(&resources.m_uav.m_readback_heap_properties, D3D12_HEAP_FLAG_NONE, &resources.m_uav.m_readback_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resources.m_uav.m_read_back_resource)) >> CHK;
	// Root signature embed in the shader already
	dx_context.GetDevice()->CreateRootSignature(0, resources.m_compute_shader->GetBufferPointer(), resources.m_compute_shader->GetBufferSize(), IID_PPV_ARGS(&resources.m_compute_root_signature)) >> CHK;

	D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc =
	{
		.pRootSignature = resources.m_compute_root_signature.Get(),
		.CS = 
		{
			.pShaderBytecode = resources.m_compute_shader->GetBufferPointer(),
			.BytecodeLength = resources.m_compute_shader->GetBufferSize(),
		},
	};
	dx_context.GetDevice()->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&resources.m_compute_pso)) >> CHK;
	return resources;
}

void FillCommandList
(
	DXContext& dx_context, 
	DXWindow& dx_window, const Resources& resource, 
	const D3D12_VERTEX_BUFFER_VIEW& vertex_buffer_view, uint32 vertex_count
)
{
	// Fill CommandList
	dx_window.BeginFrame(dx_context);
	{
		// Compute Work
		{
			dx_context.GetCommandListGraphics()->SetComputeRootSignature(resource.m_compute_root_signature.Get());
			dx_context.GetCommandListGraphics()->SetPipelineState(resource.m_compute_pso.Get());
			dx_context.GetCommandListGraphics()->SetComputeRootUnorderedAccessView(0, resource.m_uav.m_gpu_resource->GetGPUVirtualAddress());
			dx_context.GetCommandListGraphics()->Dispatch(resource.m_dispatch_count, 1, 1);
			D3D12_RESOURCE_BARRIER barriers[1]{};
			barriers[0] =
			{
				.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Transition =
				{
					.pResource = resource.m_uav.m_gpu_resource.Get(),
					.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
					.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE,
				},
			};
			dx_context.GetCommandListGraphics()->ResourceBarrier(COUNT(barriers), &barriers[0]);
			dx_context.GetCommandListGraphics()->CopyResource(resource.m_uav.m_read_back_resource.Get(), resource.m_uav.m_gpu_resource.Get());
		}
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
	dx_window.EndFrame(dx_context);
}

void CreateSetup
(
	DXContext& dx_context, const DXCompiler& dx_compiler, const DXWindow& dx_window, 
	Resources& resource, 
	ComPtr<ID3D12Resource2>& vertex_buffer, D3D12_VERTEX_BUFFER_VIEW& vertex_buffer_view, 
	uint32& vertex_count
)
{
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
		// TODO do we need to transition to proper state?
		dx_context.GetDevice()->CreateCommittedResource
		(
			&heap_properties, D3D12_HEAP_FLAG_NONE, &vertex_desc,
			D3D12_RESOURCE_STATE_COMMON, nullptr,
			IID_PPV_ARGS(&vertex_buffer)
		) >> CHK;
		NAME_DX_OBJECT(vertex_buffer, "VertexBuffer");

		const D3D12_HEAP_PROPERTIES heap_properties_upload =
		{
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1,
		};
		ComPtr<ID3D12Resource2> vertex_upload_buffer{};
		dx_context.GetDevice()->CreateCommittedResource
		(
			&heap_properties_upload, D3D12_HEAP_FLAG_NONE, &vertex_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&vertex_upload_buffer)
		) >> CHK;
		NAME_DX_OBJECT(vertex_upload_buffer, "VertexUploadBuffer");
		Vertex* data = nullptr;
		vertex_upload_buffer->Map(0, nullptr, reinterpret_cast<void**>(&data)) >> CHK;
		memcpy(data, vertex_data, sizeof(vertex_data));
		vertex_upload_buffer->Unmap(0, nullptr);
		dx_context.InitCommandLists();
		dx_context.GetCommandListGraphics()->CopyResource(vertex_buffer.Get(), vertex_upload_buffer.Get());
		D3D12_RESOURCE_BARRIER barriers[1]{};
		barriers[0] =
		{
			.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Transition =
			{
				.pResource = vertex_buffer.Get(),
				.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
				.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
			},
		};
		dx_context.GetCommandListGraphics()->ResourceBarrier(_countof(barriers), &barriers[0]);
		// Copy queue has some constraints regarding copy state and barriers
		// Compute has synchronization issue
		dx_context.ExecuteCommandListGraphics();

		vertex_buffer_view =
		{
			.BufferLocation = vertex_buffer->GetGPUVirtualAddress(),
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

int main()
{
	MemoryTrackStart();

	DXReportContext dx_report_context{};
	{
		State state{};
		// TODO PIX / renderdoc markers
		const GRAPHICS_DEBUGGER_TYPE gd_type{ GRAPHICS_DEBUGGER_TYPE::NONE};
		GPUCapture gpu_capture(gd_type);
		DXContext dx_context{};
		dx_report_context.SetDevice(dx_context.GetDevice());
		DXCompiler dx_compiler("shaders");
		DXWindowManager window_manager;
		{
			uint32 width = 600;
			uint32 height = 500;
			WindowDesc window_desc =
			{
				.m_window_name = { "Playground" },
				.m_width = width,
				.m_height = height,
				.m_origin_x = (1920 >> 1) - (width >> 1),
				.m_origin_y = (1080 >> 1) - (height >> 1),
			};
			DXWindow dx_window(dx_context, window_manager, &state, window_desc);
			window_desc.m_window_name = "Playground 1";
			DXWindow dx_window1(dx_context, window_manager, &state, window_desc);
			{
				Resources resource = CreateResources(dx_context, dx_compiler);
				ComPtr<ID3D12Resource2> vertex_buffer{};
				D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view{};
				uint32 vertex_count{};
				CreateSetup(dx_context, dx_compiler, dx_window, resource, vertex_buffer, vertex_buffer_view, vertex_count);

				Resources resource1 = CreateResources(dx_context, dx_compiler);
				ComPtr<ID3D12Resource2> vertex_buffer1{};
				D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view1{};
				uint32 vertex_count1{};
				CreateSetup(dx_context, dx_compiler, dx_window1, resource1, vertex_buffer1, vertex_buffer_view1, vertex_count1);

				while (!dx_window.ShouldClose() && !dx_window1.ShouldClose())
				{
					// Process window message
					dx_window.Update();
					dx_window1.Update();

					if (state.m_capture)
					{
						gpu_capture.PIXCaptureAndOpen();
						gpu_capture.RenderdocCaptureStart();
					}
					// rendering
					{
						// Handle resizing
						if (dx_window.ShouldResize())
						{
							dx_context.Flush(dx_window.GetBackBufferCount());
							dx_window.Resize(dx_context);
						}

						if (dx_window1.ShouldResize())
						{
							dx_context.Flush(dx_window1.GetBackBufferCount());
							dx_window1.Resize(dx_context);
						}

						dx_context.InitCommandLists();
						FillCommandList(dx_context, dx_window, resource, vertex_buffer_view, vertex_count);
						FillCommandList(dx_context, dx_window1, resource1, vertex_buffer_view1, vertex_count1);
						dx_context.ExecuteCommandListGraphics();
						dx_window.Present();
						dx_window1.Present();

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
					if (state.m_capture)
					{
						gpu_capture.RenderdocCaptureEnd();
						state.m_capture = !state.m_capture;
					}

				}
				dx_context.Flush(dx_window.GetBackBufferCount());
			}
		}
	}
	
	return 0;
}