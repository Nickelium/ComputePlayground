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
VSOutput main(in const VSInput input) 
{
	VSOutput output = (VSOutput)0;
	output.pos = float4(input.pos,0,1);
	output.color = input.color;
	return output;
}