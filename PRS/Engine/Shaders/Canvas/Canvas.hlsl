// Canvas / UI shader
#include "Include/Common.hlsli"

struct VSInput { float2 pos : POSITION; float2 uv : TEXCOORD0; float4 col : COLOR; };
struct VSOutput { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; float4 col : COLOR; };

VSOutput VSMain(VSInput i) {
    VSOutput o;
    o.pos = float4(i.pos, 0, 1);
    o.uv = i.uv;
    o.col = i.col;
    return o;
}

float4 PSMain(VSOutput i) : SV_TARGET {
    return i.col * (g_Texture.Sample(g_Sampler, i.uv));
}