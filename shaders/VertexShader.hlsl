#include "Common.hlsl"

struct VSInput
{
	float2 pos : Position;
	float3 color : Color;
};

struct VSOutput
{
	float4 pos : SV_Position;
	float3 color : COLOR;
};

[RootSignature("RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)")]
VSOutput main(in const VSInput input, uint id : SV_VERTEXID) 
{
	VSOutput output = (VSOutput)0;
	//if(id == 0)
	//output.pos = float4(0.5, -0.5,0,1);
	//if(id == 1)
	//output.pos = float4(-0.5, 0.5,0,1);
	//if(id == 2)
	//output.pos = float4(-0.5, -0.5,0,1);

    output.pos = float4(input.pos, 0, 1);
    output.color = input.color;
	return output;
}