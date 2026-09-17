cbuffer GridConstant : register(b0)
{
    row_major float4x4 MVP;
    row_major float4x4 World;
    float CellSize;
    float3 Padding;
}

struct PS_INPUT
{
    float4 Position : SV_Position;
    float3 WorldPos : TEXCOORD0;
};

float4 MainPS(PS_INPUT Input) : SV_Target
{
    float2 p = Input.WorldPos.xy;

    float2 cell = floor(p / CellSize);
    float parity = frac((cell.x + cell.y) * 0.5) * 2.0;

    float3 dark = float3(0.15, 0.15, 0.16); 
    float3 light = float3(0.55, 0.55, 0.58);
    float3 col = (parity < 0.5) ? dark : light;

    return float4(col, 1.0);
}