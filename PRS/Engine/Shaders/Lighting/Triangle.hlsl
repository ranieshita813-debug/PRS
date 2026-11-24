struct VSOut { float4 pos : SV_Position; float3 color : COLOR0; };

VSOut VSMain(uint id : SV_VertexID)
{
    float2 p[3] = { float2(0.0, -0.5), float2(0.5, 0.5), float2(-0.5, 0.5) };
    float3 c[3] = { float3(1,0,0), float3(0,1,0), float3(0,0,1) };
    VSOut o; o.pos = float4(p[id], 0, 1); o.color = c[id]; return o;
}
float4 PSMain(VSOut i) : SV_Target { return float4(i.color, 1); }
