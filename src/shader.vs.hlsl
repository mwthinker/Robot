cbuffer VertexUniforms : register(b0, space1)
{
    float4x4 projectionMatrix;
};

struct VSInput
{
    float3 position : TEXCOORD0;
    float2 tex      : TEXCOORD1;
    float4 color    : TEXCOORD2;
    float3 normal   : TEXCOORD3;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 tex      : TEXCOORD0;
    float4 color    : COLOR0;
    float3 normal   : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = mul(projectionMatrix, float4(input.position, 1.0f));
    output.tex = input.tex;
    output.color = input.color;
    output.worldPos = input.position;
    output.normal = input.normal;
    return output;
}
