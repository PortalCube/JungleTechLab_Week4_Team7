#include "Constants.hlsli"

// 기즈모 및 언릿 메쉬용 픽셀 셰이더
struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float3 Normal : NORMAL;
};

float4 MainPS(PS_INPUT Input) : SV_Target
{
    float3 BaseColor = lerp(Input.Color.rgb, ColorOverride, ColorOverrideAmount);
    return float4(BaseColor, Input.Color.a);
}

float4 main(PS_INPUT Input) : SV_Target
{
    return MainPS(Input);
}
