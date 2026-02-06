Texture2D SkyboxTexture : register(t0);
SamplerState SkyboxSampler : register(s0);

cbuffer PerObject : register(b0) {
    float4x4 WVP;
};

struct VS_INPUT {
    float3 Pos : POSITION;
    float2 UV : TEXCOORD;
    float3 Normal : NORMAL;
};

struct PS_INPUT {
    float4 Pos : SV_POSITION;
    float3 TexCoord : TEXCOORD;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    
    // Use local position as direction vector
    output.TexCoord = input.Pos;
    
    // Set Z = W to force depth to 1.0 (far plane)
    output.Pos = mul(float4(input.Pos, 1.0f), WVP).xyww;
    
    return output;
}

static const float2 invAtan = float2(0.1591, 0.3183);

float4 PS(PS_INPUT input) : SV_Target {
    float3 v = normalize(input.TexCoord);
    
    // Rectilinear to Equirectangular mapping
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    
    // Scale to 0..1
    uv *= invAtan;
    uv += 0.5;
    
    // Calculate gradients manually to handle wrapping at the seam (u=0 <-> u=1)
    float2 dx = ddx(uv);
    float2 dy = ddy(uv);
    
    // If the horizontal gradient is too large (wrapping), adjust it
    if (dx.x > 0.5) dx.x -= 1.0;
    else if (dx.x < -0.5) dx.x += 1.0;
    
    if (dy.x > 0.5) dy.x -= 1.0;
    else if (dy.x < -0.5) dy.x += 1.0;
    
    // Sample texture using explicit gradients
    return SkyboxTexture.SampleGrad(SkyboxSampler, uv, dx, dy);
}
