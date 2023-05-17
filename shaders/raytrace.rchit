#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "raycommon.glsl"
#include "wavefront.glsl"
#include "random.glsl"

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
    if (pcRay.lightType == 0)
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

    vec3 rayOrigin = state.position;
    vec3 rayDirection = samplingHemisphere(prd.seed, state.tangent, state.bitangent, state.normal);

    const float p = 1 / M_PI;

    // Compute the BRDF for this ray (assuming Lambertian reflection)
    float cos_theta = dot(rayDirection, state.normal);
    vec3 BRDF = state.mat.albedo / M_PI;

    if (prd.depth < 15)
    {
        prd.depth++;
        float tMin  = 0.001;
        float tMax  = 10000.0;
        uint  flags = gl_RayFlagsOpaqueEXT;
        traceRayEXT(topLevelAS,    // acceleration structure
                    flags,         // rayFlags
                    0xFF,          // cullMask
                    0,             // sbtRecordOffset
                    0,             // sbtRecordStride
                    0,             // missIndex
                    rayOrigin,     // ray origin
                    tMin,          // ray min range
                    rayDirection,  // ray direction
                    tMax,          // ray max range
                    0              // payload (location = 0)
        );
    }
    else {
        prd.hitValue = vec3(pcRay.lightIntensity * 0.001);
    }
    vec3 incoming = prd.hitValue;

    // Apply the Rendering Equation here.
    prd.hitValue = state.mat.emission + (BRDF * incoming * cos_theta / p);
    // prd.hitValue = (state.normal + vec3(1)) * 0.5;
    // prd.hitValue = state.normal;
    // prd.hitValue = state.mat.albedo;
    // prd.hitValue = vec3(state.texCoord, 0);
}
