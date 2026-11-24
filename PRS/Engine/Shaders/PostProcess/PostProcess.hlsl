#include "Include/Common.hlsli"
float4 PSMain(float2 uv:TEXCOORD0):SV_TARGET{return float4(uv,0,1);}