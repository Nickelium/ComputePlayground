#include "Common.hlsl"

struct PSInput
{
	float4 pos : SV_Position;
	float3 color : COLOR;
};

struct PSOutput
{
	float4 color : SV_TARGET;
};

[RootSignature("RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)")]
PSOutput main(in const PSInput input) 
{
	PSOutput output = (PSOutput)0;
    output.color = float4(input.color, 1.0f);
    output.color.rgb = Reinhard(output.color.rgb);
	return output;
}