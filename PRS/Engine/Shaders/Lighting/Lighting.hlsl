#include "Include/Common.hlsli"
#include "Include/PBR.hlsli"
float4 PSMain(float3 n: NORMAL):SV_TARGET{return float4(n*0.5+0.5,1);}