#include "gltf_material.glsl"
#include "pbr.glsl"

vec3 pathtrace(Ray r,int maxDepth)
{
    //int maxDepth = 10;
    vec3 hitValue = vec3(0);
    vec3 weight = vec3(1);
    vec3 absorption = vec3(0.0);

    for (int depth = 0; depth < maxDepth; ++depth) {
        prd.hitT = INFINITY;

        uint  rayFlags = gl_RayFlagsCullBackFacingTrianglesEXT;
        float tMin     = 0.01;
        float tMax     = INFINITY;

        traceRayEXT(topLevelAS, // acceleration structure
            rayFlags,       // rayFlags
            0xFF,           // cullMask
            0,              // sbtRecordOffset
            0,              // sbtRecordStride
            0,              // missIndex
            r.origin,       // ray origin
            tMin,           // ray min range
            r.direction,    // ray direction
            tMax,           // ray max range
            0               // payload (location = 0)
        );

        /* Miss */
        if (prd.hitT == INFINITY) {
            if(depth == 0)
                hitValue = pcRay.clearColor.xyz * 0.8;
            else {
                vec3 env = pcRay.clearColor.xyz * pcRay.lightIntensity * 0.1;
                hitValue += env * weight;
            }

            return hitValue;
        }

        /* Hit */
        State state;
        getShadeState(state, prd);
        state.ffnormal = dot(state.normal, r.direction) <= 0.0 ? state.normal : -state.normal;
        createCoordinateSystem(state.ffnormal, state.tangent, state.bitangent);
        state.eta = dot(state.normal, state.ffnormal) > 0.0 ? (1.0 / state.mat.ior) : state.mat.ior;
        // return state.normal;
        // return vec3(state.mat.roughness);
        // return vec3(state.mat.metallic);
        // return state.mat.f0;
        // return vec3(state.mat.transmission);

        // Vector toward the light
        vec3  L;
        float lightIntensity = pcRay.lightIntensity;
        float lightDistance = 100000.0;
        float lightPdf = 1;
        // Point light
        if (pcRay.lightType == 0) {
            vec3 lDir = pcRay.lightPosition - state.position;
            lightDistance = length(lDir);
            lightIntensity = pcRay.lightIntensity / (lightDistance * lightDistance * 0.2);
            L = normalize(-lDir);
        }
        else { // Directional light
            L = normalize(pcRay.lightPosition);
        }

        // Reset absorption when ray is going out of surface
        if(dot(state.normal, state.ffnormal) > 0.0) {
            absorption = vec3(0.0);
        }
        
        // Emission
        hitValue += state.mat.emission * weight;
        
        // Add absoption (transmission / volume)
        // weight *= exp(-absorption * prd.hitT);

        vec3 Li = vec3(0);
        if (dot(state.ffnormal, L) < 0) {
            BsdfSampleRec directBsdf;
            directBsdf.f = PbrEval(state, -r.direction, state.ffnormal, -L, directBsdf.pdf);
            float misWeight = max(0, powerHeuristic(lightPdf, directBsdf.pdf)); // multi importance sampling weight
            Li = misWeight * directBsdf.f * abs(dot(-L, state.ffnormal)) * lightIntensity * weight / lightPdf;
        }

        BsdfSampleRec bsdf;
        bsdf.f = PbrSample(state, -r.direction, state.ffnormal, bsdf.L, bsdf.pdf, prd.seed);
        // bsdf.f = vec3(0);
        // return bsdf.L * 0.5 + 0.5;

        weight *= bsdf.f * abs(dot(state.ffnormal, bsdf.L)) / bsdf.pdf;

        // Direct light
        if (dot(state.ffnormal, L) < 0) {
            shadow_prd.isShadowed = true; // Asume hit, will be set to false if hit nothing (miss shader)
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
                        lightDistance,         // ray max range
                        1             // payload layout(location = 1)
            );

            if (!shadow_prd.isShadowed) {
                hitValue += Li;
            }
        }
        
        r.origin = state.position;
        r.direction = bsdf.L;
    }

    return hitValue;
}

vec3 samplePixel(ivec2 imageCoord, ivec2 imageSize,int maxDepth)
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
    vec3 radiance = pathtrace(r,maxDepth);

    return radiance;
}