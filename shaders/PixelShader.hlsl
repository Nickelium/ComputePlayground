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

[RootSignature(ROOTFLAGS_DEFAULT ", RootConstants(num32BitConstants=1, b0)")]
PSOutput main(in const PSInput input) 
{
	PSOutput output = (PSOutput)0;
    output.color = float4(input.color, 1.0f);
    output.color.rgb = Reinhard(output.color.rgb);
	//output.color = float4(1, 0, 0, 1);
	return output;
}