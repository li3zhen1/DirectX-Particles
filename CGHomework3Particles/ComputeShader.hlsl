
#define BLOCK_SIZE 128

static float softeningSquared = 0.0012500000 * 0.0012500000;
static float g_fG = 6.67300e-11f * 10000.0f;
static float g_fParticleMass = g_fG * 10000.0f * 10000.0f;

groupshared float4 sharedPos[BLOCK_SIZE];

// 更新 bi
void bodyBodyInteraction(inout float3 ai, float4 bj, float4 bi, float mass, int particles) {
    float3 r = bj.xyz - bi.xyz;

    float distSqr = dot(r, r);
    distSqr += softeningSquared;

    float invDist = 1.0f / sqrt(distSqr);
    float invDistCube = invDist * invDist * invDist;

    float s = mass * invDistCube * particles;

    ai += r * s;
}

// pcbCS->param = {MAX_PARTICLES, dimx}, paramf = {0.1f, 1}
cbuffer cbCS : register(b0) {
    uint4   g_param;            
    float4  g_paramf;
};

struct PosVelo {
    float4 pos;
    float4 velo;
};

StructuredBuffer<PosVelo> oldPosVelo;
RWStructuredBuffer<PosVelo> newPosVelo;

[numthreads(BLOCK_SIZE, 1, 1)]
void main(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex) {
    float4 pos = oldPosVelo[DTid.x].pos;
    float4 vel = oldPosVelo[DTid.x].velo;
    float3 accel = 0;
    float mass = g_fParticleMass;

    // 更新
    [loop]
    for (uint tile = 0; tile < g_param.y; tile++)
    {
        // 缓存 pos
        sharedPos[GI] = oldPosVelo[tile * BLOCK_SIZE + GI].pos;
        GroupMemoryBarrierWithGroupSync();

        [unroll]
        for (uint counter = 0; counter < BLOCK_SIZE; counter += 8)
        {
            bodyBodyInteraction(accel, sharedPos[counter], pos, mass, 1);
            bodyBodyInteraction(accel, sharedPos[counter + 1], pos, mass, 1);
            bodyBodyInteraction(accel, sharedPos[counter + 2], pos, mass, 1);
            bodyBodyInteraction(accel, sharedPos[counter + 3], pos, mass, 1);
            bodyBodyInteraction(accel, sharedPos[counter + 4], pos, mass, 1);
            bodyBodyInteraction(accel, sharedPos[counter + 5], pos, mass, 1);
            bodyBodyInteraction(accel, sharedPos[counter + 6], pos, mass, 1);
            bodyBodyInteraction(accel, sharedPos[counter + 7], pos, mass, 1);
        }

        GroupMemoryBarrierWithGroupSync();
    }

    const uint tooManyParticles = g_param.y * BLOCK_SIZE - g_param.x;
    bodyBodyInteraction(accel, float4(0, 0, 0, 0), pos, mass, -tooManyParticles);

    vel.xyz += accel.xyz * g_paramf.x;
    vel.xyz *= g_paramf.y;
    pos.xyz += vel.xyz * g_paramf.x;  

    if (DTid.x < g_param.x)
    {
        newPosVelo[DTid.x].pos = pos;
        newPosVelo[DTid.x].velo = float4(vel.xyz, length(accel));
    }
}
