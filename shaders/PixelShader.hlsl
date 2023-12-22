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
PSOutput main(in PSInput psInput) 
{
	PSOutput psOutput = (PSOutput)0;
	psOutput.color = float4(psInput.color, 1.0f);
	return psOutput;
}