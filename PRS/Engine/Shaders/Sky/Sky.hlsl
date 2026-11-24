// Sky shader (procedural sky + sun)
#include "Include/Common.hlsli"
#include "Include/PBR.hlsli"

struct VSInput { float3 pos : POSITION; };
struct VSOutput { float4 pos : SV_POSITION; float3 dir : TEXCOORD0; };

VSOutput VSMain(VSInput i) {
    VSOutput o;
    o.pos = mul(float4(i.pos,1), g_ViewProj);
    o.dir = i.pos;
    return o;
}

float4 PSMain(VSOutput i) : SV_TARGET {
    float3 d = normalize(i.dir);
    float t = saturate(d.y * 0.5 + 0.5);
    float3 sky = lerp(float3(0.8,0.9,1.0), float3(0.1,0.3,0.8), t);
    float sun = pow(saturate(dot(d, normalize(g_SunDir))), 256.0);
    return float4(sky + sun, 1);
}