// Skeleton skinning shader
#include "Include/Common.hlsli"

struct VSInput { float3 pos : POSITION; float3 nrm : NORMAL; float2 uv : TEXCOORD0; uint4 bones : BLENDINDICES; float4 weights : BLENDWEIGHT; };
struct VSOutput { float4 pos : SV_POSITION; float3 nrm : NORMAL; float2 uv : TEXCOORD0; };

VSOutput VSMain(VSInput i) {
    float4x4 skin = g_BoneMatrices[i.bones.x]*i.weights.x + g_BoneMatrices[i.bones.y]*i.weights.y + g_BoneMatrices[i.bones.z]*i.weights.z + g_BoneMatrices[i.bones.w]*i.weights.w;
    VSOutput o;
    o.pos = mul(float4(i.pos,1), mul(skin, g_ViewProj));
    o.nrm = normalize(mul(i.nrm, (float3x3)skin));
    o.uv = i.uv;
    return o;
}

float4 PSMain(VSOutput i) : SV_TARGET { return float4(i.nrm*0.5+0.5,1); }