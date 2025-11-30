struct VSOutput
{
    float4 position   : SV_Position;
    float2 tex        : TEXCOORD0;
    float4 color      : COLOR0;
    float3 worldPos   : TEXCOORD1;
};

Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

cbuffer LightData : register(b0, space3)
{
    float4 lightPos;
    float4 lightColor;
    float4 params; // x = radius, y = ambientStrength
};

float4 main(VSOutput input) : SV_Target
{
    float4 baseColor =
        (input.tex.x >= 0 && input.tex.y >= 0)
        ? Texture.Sample(Sampler, input.tex) * input.color
        : input.color;

    float lightRadius = params.x;
    float ambientStrength = params.y;

    float3 toLight = lightPos.xyz - input.worldPos;
    float dist = length(toLight);

    float attenuation = saturate(1.0 - dist / lightRadius);
    float3 diffuse = lightColor.rgb * attenuation;
    float3 ambient = lightColor.rgb * ambientStrength;

    float3 lit = baseColor.rgb * (ambient + diffuse);

    return float4(lit, baseColor.a);
}
