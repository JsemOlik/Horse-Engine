Texture2D MainTex : register(t0);
SamplerState MainSampler : register(s0);

cbuffer Object : register(b0) {
    float4x4 WVP;
};

cbuffer Material : register(b1) {
    float4 AlbedoColor;
    float Roughness;
    float Metalness;
    float2 Padding;
};

struct VS_INPUT {
    float3 Pos : POSITION;
    float4 Color : COLOR;
    float2 UV : TEXCOORD;
};

struct PS_INPUT {
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
    float2 UV : TEXCOORD;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.Pos = mul(float4(input.Pos, 1.0f), WVP);
    output.Color = input.Color;
    output.UV = input.UV;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    return MainTex.Sample(MainSampler, input.UV) * input.Color * AlbedoColor;
}
