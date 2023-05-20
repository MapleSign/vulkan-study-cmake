#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "raycommon.glsl"

layout(location = 1) rayPayloadInEXT ShadowHitPayload shadow_prd;

void main()
{
    shadow_prd.isShadowed = false;
}
