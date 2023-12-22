#include "Common.h"
#include "DXCompiler.h"
#include "DXContext.h"
#include "dxcapi.h" // DXC compiler
#include "DXCommon.h"
#include "DXDebugLayer.h"
#include "DXWindow.h"
#include "DXCommon.h"
#include "DXResource.h"

#include "maths/LinearAlgebra.h"
DISABLE_OPTIMISATIONS()

using namespace Microsoft::WRL; // ComPtr

struct Resources
{
	UAV m_uav;
	ComPtr<IDxcBlob> m_computeShader;
	ComPtr<ID3D12PipelineState> m_computePSO;
	ComPtr<ID3D12RootSignature> m_computeRootSignature;
	uint32_t m_kGroupSize;
	uint32_t m_kDispatchCount;

	// TODO add inputlayout
	ComPtr<IDxcBlob> m_vertexShader;
	ComPtr<IDxcBlob> m_pixelShader;
	ComPtr<ID3D12PipelineState> m_gfxPSO;
	ComPtr<ID3D12RootSignature> m_gfxRootSignature;
};

Resources CreateResources(const DXContext& dxContext)
{
	Resources resources{};
	resources.m_kGroupSize = 1024;
	resources.m_kDispatchCount = 1;

	{
		DXCompiler dxCompiler;
		bool compileDebug{};
#if defined(_DEBUG)
		compileDebug = true;
#else
		compileDebug = false;
#endif
		dxCompiler.Init(compileDebug);
		dxCompiler.Compile(&resources.m_computeShader, L"shaders/ComputeShader.hlsl", ShaderType::COMPUTE_SHADER);
		dxCompiler.Compile(&resources.m_vertexShader, L"shaders/VertexShader.hlsl", ShaderType::VERTEX_SHADER);
		dxCompiler.Compile(&resources.m_pixelShader, L"shaders/PixelShader.hlsl", ShaderType::PIXEL_SHADER);
	}

	resources.m_uav.m_Desc.Width = resources.m_kGroupSize * resources.m_kDispatchCount * sizeof(float32) * 4;
	resources.m_uav.m_ReadbackDesc.Width = resources.m_uav.m_Desc.Width;
	dxContext.GetDevice()->CreateCommittedResource(&resources.m_uav.m_HeapProperties, D3D12_HEAP_FLAG_NONE, &resources.m_uav.m_Desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&resources.m_uav.m_GPUResource)) >> CHK;
	dxContext.GetDevice()->CreateCommittedResource(&resources.m_uav.m_ReadbackHeapProperties, D3D12_HEAP_FLAG_NONE, &resources.m_uav.m_ReadbackDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resources.m_uav.m_ReadbackResource)) >> CHK;

	// Root signature embed in the shader already
	dxContext.GetDevice()->CreateRootSignature(0, resources.m_computeShader->GetBufferPointer(), resources.m_computeShader->GetBufferSize(), IID_PPV_ARGS(&resources.m_computeRootSignature)) >> CHK;

	D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc =
	{
		.pRootSignature = resources.m_computeRootSignature.Get(),
		.CS = 
		{
			.pShaderBytecode = resources.m_computeShader->GetBufferPointer(),
			.BytecodeLength = resources.m_computeShader->GetBufferSize(),
		},
	};
	dxContext.GetDevice()->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&resources.m_computePSO)) >> CHK;
	return resources;
}

IDXDebugLayer* CreateDebugLayer(GRAPHICS_DEBUGGER_TYPE gdType)
{
#if defined(_DEBUG)
	return new DXDebugLayer(gdType);
#else
	return new DXNullDebugLayer();
#endif
}

DXContext* CreateDXContext(GRAPHICS_DEBUGGER_TYPE gdType)
{
#if defined(_DEBUG)
	return new DXDebugContext(gdType == GRAPHICS_DEBUGGER_TYPE::RENDERDOC);
#else
	return new DXContext();
#endif
}

int main()
{
	_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, CrtDbgHook);
	GRAPHICS_DEBUGGER_TYPE gdType = GRAPHICS_DEBUGGER_TYPE::PIX;
	IDXDebugLayer* pDxDebugLayer = CreateDebugLayer(gdType);
	pDxDebugLayer->Init();
	{
		// load
		DXContext* pDxContext = CreateDXContext(gdType);
		pDxContext->Init();
		D3D12_FEATURE_DATA_D3D12_OPTIONS feature;
		pDxContext->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &feature, sizeof(feature)) >> CHK;

		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		descHeapDesc.NumDescriptors = 1;
		Resources resource = CreateResources(*pDxContext);
		ComPtr<ID3D12Resource2> vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		uint32_t vertexCount;
		{

			struct Vertex
			{
				float2 position;
				float3 color;
			};

			Vertex vertexData[] =
			{
				{ {+0.0f, +0.25f}, {1.0f, 0.0f, 0.0f} },
				{ {+0.25f, -0.25f}, {0.0f, 1.0f, 0.0f} },
				{ {-0.25f, -0.25f}, {0.0f, 0.0f, 1.0f} },
			};

			vertexCount = _countof(vertexData);

			uint32_t numberVertices = _countof(vertexData);
			D3D12_RESOURCE_DESC vertexDesc =
			{
				.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
				.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, // 64 * 1024 bytes = 64kB
				.Width = sizeof(vertexData),
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
			D3D12_HEAP_PROPERTIES heapProperties =
			{
				.Type = D3D12_HEAP_TYPE_DEFAULT,
				.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
				.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
				.CreationNodeMask = 1,
				.VisibleNodeMask = 1,
			};
			pDxContext->GetDevice()->CreateCommittedResource
			(
				&heapProperties, D3D12_HEAP_FLAG_NONE, &vertexDesc,
				D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
				IID_PPV_ARGS(&vertexBuffer)
			) >> CHK;
			vertexBuffer->SetName(L"VertexBuffer");

			D3D12_HEAP_PROPERTIES heapPropertiesUpload =
			{
				.Type = D3D12_HEAP_TYPE_UPLOAD,
				.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
				.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
				.CreationNodeMask = 1,
				.VisibleNodeMask = 1,
			};
			ComPtr<ID3D12Resource2> vertexUploadBuffer;
			pDxContext->GetDevice()->CreateCommittedResource
			(
				&heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &vertexDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
				IID_PPV_ARGS(&vertexUploadBuffer)
			) >> CHK;
			vertexUploadBuffer->SetName(L"VertexUploadBuffer");
			Vertex* data = nullptr;
			vertexUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&data)) >> CHK;
			memcpy(data, vertexData, sizeof(vertexData));
			vertexUploadBuffer->Unmap(0, nullptr);
			pDxContext->InitCommandList();
			pDxContext->GetCommandList()->CopyResource(vertexBuffer.Get(), vertexUploadBuffer.Get());
			D3D12_RESOURCE_BARRIER barriers[1]{};
			barriers[0] =
			{
				.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Transition =
				{
					.pResource = vertexBuffer.Get(),
					.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
					.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
					.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
				},
			};
			pDxContext->GetCommandList()->ResourceBarrier(_countof(barriers), &barriers[0]);
			pDxContext->ExecuteCommandList();

			vertexBufferView =
			{
				.BufferLocation = vertexBuffer->GetGPUVirtualAddress(),
				.SizeInBytes = sizeof(vertexData),
				.StrideInBytes = sizeof(vertexData[0]),
			};

			D3D12_INPUT_ELEMENT_DESC elementsDesc[] =
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
			D3D12_INPUT_LAYOUT_DESC layoutDesc =
			{
				.pInputElementDescs = elementsDesc,
				.NumElements = _countof(elementsDesc),
			};

			pDxContext->GetDevice()->CreateRootSignature(0, resource.m_vertexShader->GetBufferPointer(), resource.m_vertexShader->GetBufferSize(), IID_PPV_ARGS(&resource.m_gfxRootSignature)) >> CHK;
		
			D3D12_GRAPHICS_PIPELINE_STATE_DESC gfxPSODesc =
			{
				.pRootSignature = resource.m_gfxRootSignature.Get(),
				.VS = 
				{
					.pShaderBytecode = resource.m_vertexShader->GetBufferPointer(),
					.BytecodeLength = resource.m_vertexShader->GetBufferSize(),
				},
				.PS =
				{
					.pShaderBytecode = resource.m_pixelShader->GetBufferPointer(),
					.BytecodeLength = resource.m_pixelShader->GetBufferSize(),
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
				.InputLayout = layoutDesc,
				//.IBStripCutValue = nullptr,
				.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				.NumRenderTargets = 1,
				.RTVFormats = 
				{
					DXGI_FORMAT_R8G8B8A8_UNORM,
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
			pDxContext->GetDevice()->CreateGraphicsPipelineState(&gfxPSODesc, IID_PPV_ARGS(&resource.m_gfxPSO)) >> CHK;
	}

		DXWindow dxWindow;
		dxWindow.Init(*pDxContext);
		//dxWindow.SetFullScreen(false);
		while (!dxWindow.ShouldClose())
		{
			// Process window message
			dxWindow.Update();
			// Handle resizing
			if (dxWindow.ShouldResize())
			{
				pDxContext->Flush(dxWindow.GetBackBufferCount());
				dxWindow.Resize(*pDxContext);
			}

			pDxContext->InitCommandList();
			{
				// Fill CommandList
				dxWindow.BeginFrame(*pDxContext);
				// Compute Work
				{
					pDxContext->GetCommandList()->SetComputeRootSignature(resource.m_computeRootSignature.Get());
					pDxContext->GetCommandList()->SetPipelineState(resource.m_computePSO.Get());
					pDxContext->GetCommandList()->SetComputeRootUnorderedAccessView(0, resource.m_uav.m_GPUResource->GetGPUVirtualAddress());
					pDxContext->GetCommandList()->Dispatch(resource.m_kDispatchCount, 1, 1);
					D3D12_RESOURCE_BARRIER barriers[1]{};
					barriers[0] =
					{
						.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
						.Transition =
						{
							.pResource = resource.m_uav.m_GPUResource.Get(),
							.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
							.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
							.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE,
						},
					};
					pDxContext->GetCommandList()->ResourceBarrier(_countof(barriers), &barriers[0]);
					pDxContext->GetCommandList()->CopyResource(resource.m_uav.m_ReadbackResource.Get(), resource.m_uav.m_GPUResource.Get());
				}
				// Draw Work
				{
					D3D12_VIEWPORT viewPorts[] =
					{
						{
							.TopLeftX = 0,
							.TopLeftY = 0,
							.Width = (float)dxWindow.GetWidth(),
							.Height = (float)dxWindow.GetHeight(),
							.MinDepth = 0,
							.MaxDepth = 1,
						}
					};
					D3D12_RECT scissorRects[] = 
					{
						{
							.left = 0,
							.top = 0,
							.right = (long)dxWindow.GetWidth(),
							.bottom = (long)dxWindow.GetHeight(),
						}
					};
					pDxContext->GetCommandList()->RSSetViewports(1, viewPorts);
					pDxContext->GetCommandList()->RSSetScissorRects(1, scissorRects);
					pDxContext->GetCommandList()->SetPipelineState(resource.m_gfxPSO.Get());
					pDxContext->GetCommandList()->SetGraphicsRootSignature(resource.m_gfxRootSignature.Get());
					pDxContext->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
					pDxContext->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					pDxContext->GetCommandList()->DrawInstanced(vertexCount, 1, 0, 0);
				}
				dxWindow.EndFrame(*pDxContext);

			}
			pDxContext->ExecuteCommandList();
			dxWindow.Present();

			float32* data = nullptr;
			D3D12_RANGE range = { 0, resource.m_uav.m_ReadbackDesc.Width };
			resource.m_uav.m_ReadbackResource->Map(0, &range, (void**)&data);
			for (int i = 0; i < resource.m_uav.m_ReadbackDesc.Width / sizeof(float32) / 4; i++)
				printf("uav[%d] = %.3f, %.3f, %.3f, %.3f\n", i, data[i * 4 + 0], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]);
			resource.m_uav.m_ReadbackResource->Unmap(0, nullptr);
		}
		pDxContext->Flush(dxWindow.GetBackBufferCount());
		dxWindow.Close();
		delete pDxContext;

	}
	pDxDebugLayer->Close();
	delete pDxDebugLayer;
	return 0;
}