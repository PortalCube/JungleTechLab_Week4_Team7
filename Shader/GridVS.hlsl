cbuffer GridConstant : register(b0)
{
    row_major float4x4 MVP;
    row_major float4x4 World;
    float CellSize;
    float3 Padding;
}

struct VS_INPUT
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float3 Normal : NORMAL;
};

struct PS_INPUT
{
    float4 Position : SV_Position;
    float3 WorldPos : TEXCOORD0;
};

PS_INPUT MainVS(VS_INPUT Input)
{
    PS_INPUT Output;
    float4 Local = float4(Input.Position, 1.0f);

    Output.Position = mul(Local, MVP);
    Output.WorldPos = mul(Local, World).xyz;
    return Output;
}