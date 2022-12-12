
struct VSParticleIn {
    float4  color   : COLOR;
    uint    id      : SV_VERTEXID;
};

struct VSParticleDrawOut {
    float3 pos			: POSITION;
    float4 color		: COLOR;
};

struct GSParticleDrawOut {
    float2 tex			: TEXCOORD0;
    float4 color		: COLOR;
    float4 pos			: SV_POSITION;
};

struct PSParticleDrawIn {
    float2 tex			: TEXCOORD0;
    float4 color		: COLOR;
};

struct PosVelo {
    float4 pos;
    float4 velo;
};

Texture2D		            g_txDiffuse;
StructuredBuffer<PosVelo>   g_bufPosVelo;


SamplerState g_samLinear {
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

cbuffer cb0 {
    row_major float4x4 g_mWorldViewProj;
    row_major float4x4 g_mInvView;
};

cbuffer cb1 {
    static float g_fParticleRad = 10.0f;
};

cbuffer cbImmutable {
    static float3 g_positions[4] =
    {
        float3(-1, 1, 0),
        float3(1, 1, 0),
        float3(-1, -1, 0),
        float3(1, -1, 0),
    };

    static float2 g_texcoords[4] =
    {
        float2(0,0),
        float2(1,0),
        float2(0,1),
        float2(1,1),
    };
};

// 粒子绘制
VSParticleDrawOut main(VSParticleIn input) {
    VSParticleDrawOut output;

    output.pos = g_bufPosVelo[input.id].pos;

    float mag = g_bufPosVelo[input.id].velo.w / 9;
    output.color = lerp(float4(0.3, 0.4, 0.9, 1), input.color, mag);

    return output;
}

// 把粒子换成两个三角形面片
[maxvertexcount(4)]
void GSParticleDraw(point VSParticleDrawOut input[1], inout TriangleStream<GSParticleDrawOut> SpriteStream) {
    GSParticleDrawOut output;
    for (int i = 0; i < 4; i++)
    {
        float3 position = g_positions[i] * g_fParticleRad;
        position = mul(position, (float3x3)g_mInvView) + input[0].pos;
        output.pos = mul(float4(position, 1.0), g_mWorldViewProj);

        output.color = input[0].color;
        output.tex = g_texcoords[i];
        SpriteStream.Append(output);
    }
    SpriteStream.RestartStrip();
}

float4 PSParticleDraw(PSParticleDrawIn input) : SV_Target {
    return g_txDiffuse.Sample(g_samLinear, input.tex) * input.color;
}