Texture2D MainTex : register(t0);
SamplerState MainSampler : register(s0);

cbuffer Object : register(b0) {
    float4x4 WVP;
};

cbuffer Material : register(b1) {
    float4 AlbedoColor;
    float Roughness;
    float Metalness;
    int ViewMode; // 0=Lit, 1=Wireframe, 2=ColoredTriangles
    float Padding;
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

float4 PS(PS_INPUT input, uint primitiveID : SV_PrimitiveID) : SV_Target {
    if (ViewMode == 2) {
        // Colored Triangles: Vibrant colors (Hot pink, Lime, etc.)
        // Use HSV -> RGB conversion with high Saturation/Value
        uint h = primitiveID * 2654435761u; // Knuth's Multiplicative Hash
        
        // Hue: 0.0 - 1.0 (Full spectrum)
        float hue = float(h & 0xFFFFFF) / 16777215.0f;
        
        // Saturation: 0.8 - 1.0 (Very vivid)
        float sat = 0.8f + 0.2f * (float((h >> 16) & 0xFF) / 255.0f);
        
        // Value: 0.9 - 1.0 (Bright)
        float val = 0.9f + 0.1f * (float((h >> 8) & 0xFF) / 255.0f);

        // Fast HSV to RGB (Hue is 0..1)
        float3 rgb = abs(hue * 6.0f - float3(3.0f, 2.0f, 4.0f));
        rgb = float3(1.0f, 1.0f, 1.0f) - saturate(rgb - 1.0f); // 2 - abs(...) logic simplified
        // The standard HUE to RGB:
        // R = abs(H * 6 - 3) - 1
        // G = 2 - abs(H * 6 - 2)
        // B = 2 - abs(H * 6 - 4)
        // Re-implementing correctly:
        float R = abs(hue * 6.0f - 3.0f) - 1.0f;
        float G = 2.0f - abs(hue * 6.0f - 2.0f);
        float B = 2.0f - abs(hue * 6.0f - 4.0f);
        float3 pureColor = saturate(float3(R, G, B));
        
        float3 finalColor = ((pureColor - 1.0f) * sat + 1.0f) * val;
        
        return float4(finalColor, 1.0f);
    }
    return MainTex.Sample(MainSampler, input.UV) * input.Color * AlbedoColor;
}
