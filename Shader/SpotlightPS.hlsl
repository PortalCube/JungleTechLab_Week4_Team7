#include "Constants.hlsli"

struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float3 ClipNormal : NORMAL;
};

float4 MainPS(PS_INPUT Input) : SV_Target
{
    // 거리 감쇠 연산
    float DistanceFactor = saturate(1.0f - Input.UV.y);
    float DistanceFalloff = pow(DistanceFactor, 2.5f);

    // 원뿔 가장자리 감쇠
    float3 Normal = normalize(Input.ClipNormal);
    float EdgeFactor = saturate(abs(Normal.z));
    EdgeFactor = smoothstep(0.0f, 0.8f, EdgeFactor);
    EdgeFactor = pow(EdgeFactor, 0.65f);

    // 기본 색상
    float3 BaseColor = lerp(Input.Color.rgb, ColorOverride, ColorOverrideAmount);

    // 중심부 발광
    float CoreGlow = pow(DistanceFactor, 5.0f);
    float3 CoreColor = lerp(BaseColor, float3(1.0f, 1.0f, 1.0f), CoreGlow * 0.45f);

    // 빛의 세기 연산
    float LightIntensity = DistanceFalloff * EdgeFactor;
    float GlowIntensity = CoreGlow * EdgeFactor * 0.8f;

    // 기본 빛 합성
    float3 FinalColor = CoreColor * LightIntensity;
    FinalColor += float3(1.0f, 1.0f, 1.0f) * GlowIntensity;

    // 최종 투명도
    float FinalAlpha = saturate(LightIntensity * 0.65f + GlowIntensity * 0.35f);

    return float4(FinalColor, FinalAlpha);
}

