#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT HitPayload prd;
hitAttributeEXT vec2 attribs;

vec3 interpolate(vec3 a, vec3 b, vec3 c, vec3 barycentrics) {
    return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

mat3 calcTBN(vec3 T, vec3 B, vec3 N) {
    T = normalize(vec3(gl_ObjectToWorldEXT * vec4(T, 0.0)));
    B = normalize(vec3(gl_ObjectToWorldEXT * vec4(B, 0.0)));
    N = normalize(vec3(gl_ObjectToWorldEXT * vec4(N, 0.0)));
    return mat3(T, B, N);
}

void main()
{
    prd.hitT = gl_HitTEXT;
    prd.primitiveID = gl_PrimitiveID;
    prd.instanceID = gl_InstanceID;
    prd.instanceCustomIndex = gl_InstanceCustomIndexEXT;
    prd.baryCoord = attribs;
    prd.objectToWorld = gl_ObjectToWorldEXT;
    prd.worldToObject = gl_WorldToObjectEXT;
}
