#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "raycommon.glsl"
#include "wavefront.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(location = 1) rayPayloadEXT bool isShadowed;
hitAttributeEXT vec2 attribs;

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices { ivec3 i[]; }; // Triangle indices
layout(buffer_reference, scalar) buffer Materials { GltfMaterial m[]; }; // Array of all materials on an object
layout(buffer_reference, scalar) buffer MatIndices { int i[]; }; // Material ID for each triangle

layout(set = 0, binding = eTlas) uniform accelerationStructureEXT topLevelAS;
layout(set = 1, binding = eObjDescs, scalar) buffer ObjDescBuffer { ObjDesc i[]; } objDesc;
layout(set = 1, binding = eTextures) uniform sampler2D[] textureSampler;

layout(push_constant) uniform _PushConstantRay { PushConstantRay pcRay; };

vec3 interpolate(vec3 a, vec3 b, vec3 c, vec3 barycentrics) {
    return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

mat3 calcTBN(vec3 T, vec3 B, vec3 N) {
    T = normalize(vec3(gl_ObjectToWorldEXT * vec4(T, 0.0)));
    B = normalize(vec3(gl_ObjectToWorldEXT * vec4(B, 0.0)));
    N = normalize(vec3(gl_ObjectToWorldEXT * vec4(N, 0.0)));
    return mat3(T, B, N);
}

#include "gltf_material.glsl"

void main()
{
    State state;
    getShadeState(state);

    // Vector toward the light
    vec3  L;
    float lightIntensity = pcRay.lightIntensity;
    float lightDistance = 100000.0;
    // Point light
    if(pcRay.lightType == 0)
    {
        vec3 lDir = pcRay.lightPosition - state.position;
        lightDistance = length(lDir);
        lightIntensity = pcRay.lightIntensity / (lightDistance * lightDistance);
        L = normalize(lDir);
    }
    else  // Directional light
    {
        L = normalize(pcRay.lightPosition);
    }

    float attenuation = 1;
    vec3 diffuse = computeDiffuse(state.mat.albedo, state.mat.albedo * 0.1, L, state.normal);
    vec3 specular;

    // Tracing shadow ray only if the light is visible from the surface
    if(dot(state.normal, L) > 0)
    {
        float tMin   = 0.001;
        float tMax   = lightDistance;
        vec3  origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
        vec3  rayDir = L;
        uint  flags =
            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
        isShadowed = true;
        traceRayEXT(topLevelAS,  // acceleration structure
                flags,       // rayFlags
                0xFF,        // cullMask
                0,           // sbtRecordOffset
                0,           // sbtRecordStride
                1,           // missIndex
                origin,      // ray origin
                tMin,        // ray min range
                rayDir,      // ray direction
                tMax,        // ray max range
                1            // payload (location = 1)
        );

        if(isShadowed) {
            attenuation = 0.3;
            specular = vec3(0);
        }
        else {
            // Specular
            // specular = computeSpecular(state.mat.specular, state.mat.glossiness, -gl_WorldRayDirectionEXT, L, nrm);
        }
    }
    else {
        specular = vec3(0);
    }

    prd.hitValue = vec3(lightIntensity * attenuation * (diffuse + specular));
}
