#include "gltf_material.glsl"

vec3 pathtrace(Ray r)
{
    int maxDepth = 3;
    vec3 hitValue = vec3(0);
    vec3 weight = vec3(1);

    for (int depth = 0; depth < maxDepth; ++depth) {
        prd.hitT = INFINITY;

        uint  rayFlags = gl_RayFlagsCullBackFacingTrianglesEXT;
        float tMin     = 0.001;
        float tMax     = 10000.0;

        traceRayEXT(topLevelAS, // acceleration structure
            rayFlags,       // rayFlags
            0xFF,           // cullMask
            0,              // sbtRecordOffset
            0,              // sbtRecordStride
            0,              // missIndex
            r.origin,     // ray origin
            tMin,           // ray min range
            r.direction,  // ray direction
            tMax,           // ray max range
            0               // payload (location = 0)
        );

        /* Miss */
        if (prd.hitT == INFINITY) {
            if(depth == 0)
                hitValue = pcRay.clearColor.xyz * 0.8;
            else {
                vec3 env = vec3(0.01);
                hitValue += env * weight;
            }

            return hitValue;
        }

        /* Hit */
        State state;
        getShadeState(state, prd);

        // Vector toward the light
        vec3  L;
        float lightIntensity = pcRay.lightIntensity;
        float lightDistance = 100000.0;
        // Point light
        if (pcRay.lightType == 0) {
            vec3 lDir = pcRay.lightPosition - state.position;
            lightDistance = length(lDir);
            lightIntensity = pcRay.lightIntensity / (lightDistance * lightDistance);
            L = normalize(lDir);
        }
        else { // Directional light
            L = normalize(pcRay.lightPosition);
        }

        vec3 newRayOrigin = state.position;
        vec3 newRayDirection = samplingHemisphere(prd.seed, state.tangent, state.bitangent, state.normal);

        const float p = 1 / M_PI;

        // Compute the BRDF for this ray (assuming Lambertian reflection)
        float cos_theta = dot(newRayDirection, state.normal);
        vec3 BRDF = state.mat.albedo / M_PI;

        hitValue += state.mat.emission * weight;
        weight *= BRDF * cos_theta / p;
        r.origin = newRayOrigin;
        r.direction = newRayDirection;

        // Direct light
        if (dot(state.normal, L) < 0) {
            isShadowed = true; // Asume hit, will be set to false if hit nothing (miss shader)
            uint rayFlags = 
                gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsCullBackFacingTrianglesEXT;

            traceRayEXT(topLevelAS,   // acceleration structure
                        rayFlags,     // rayFlags
                        0xFF,         // cullMask
                        0,            // sbtRecordOffset
                        0,            // sbtRecordStride
                        1,            // missIndex
                        state.position,     // ray origin
                        0.0,          // ray min range
                        -L,            // ray direction
                        tMax,         // ray max range
                        1             // payload layout(location = 1)
            );

            if (!isShadowed) {
                hitValue += lightIntensity * weight;
            }
        }
    }

    return hitValue;
}

vec3 samplerPixel(ivec2 imageCoord, ivec2 imageSize)
{
    float r1 = rnd(prd.seed);
    float r2 = rnd(prd.seed);
    // Subpixel jitter: send the ray through a different position inside the pixel
    // each time, to provide antialiasing.
    vec2 subpixel_jitter = vec2(r1, r2);

    const vec2 pixelCenter = vec2(imageCoord) + subpixel_jitter;
    const vec2 inUV = pixelCenter / vec2(imageSize);
    vec2 d = inUV * 2.0 - 1.0;

    vec4 origin = uni.viewInverse * vec4(0, 0, 0, 1);
    vec4 target = uni.projInverse * vec4(d.x, d.y, 1, 1);
    vec4 direction = uni.viewInverse * vec4(normalize(target.xyz), 0);

    Ray r = Ray(origin.xyz, direction.xyz);
    vec3 radiance = pathtrace(r);

    return radiance;
}