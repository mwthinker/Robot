struct VSOutput
{
    float4 position   : SV_Position;
    float2 tex        : TEXCOORD0;
    float4 color      : COLOR0;
    float3 normal     : TEXCOORD1;
    float3 worldPos   : TEXCOORD2;
};

Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

struct Light
{
    float4 position;   // xyz = world pos
    float4 color;      // rgb = color
    float4 params;     // x = radius, y = ambientStrength, z = shininess
};

cbuffer LightData : register(b0, space3)
{
    Light lights[4];
    uint  lightCount;
    float4 cameraPos;
};

float3 accumulateLighting(VSOutput input)
{
    float3 totalLight = 0.0;

    float3 N = normalize(input.normal);
    float3 V = normalize(cameraPos - input.worldPos);

    for (uint i = 0; i < lightCount; i++)
    {
        Light Lgt = lights[i];

        float radius    = Lgt.params.x;
        float ambientS  = Lgt.params.y;
        float shininess = Lgt.params.z;

        float3 toLight = Lgt.position.xyz - input.worldPos;
        float dist = length(toLight);
        float attenuation = saturate(1.0 - dist / radius);

        float3 L = normalize(toLight);
        float3 H = normalize(L + V);

        float NdotL = max(dot(N, L), 0.0);

        float3 diffuse  = Lgt.color.rgb * NdotL * attenuation;
        float3 specular = Lgt.color.rgb * pow(max(dot(N, H), 0.0), shininess) * attenuation;
        float3 ambient  = Lgt.color.rgb * ambientS;

        totalLight += ambient + diffuse + specular;
    }

    return totalLight;
}

float4 main(VSOutput input) : SV_Target
{
    float4 baseColor =
        (input.tex.x >= 0 && input.tex.y >= 0)
        ? Texture.Sample(Sampler, input.tex) * input.color
        : input.color;

    float3 lighting = accumulateLighting(input);
    float3 litColor = baseColor.rgb * lighting;

    return float4(litColor, baseColor.a);
}
