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
                hitValue = texture(envSampler, r.direction).rgb;
            else {
                vec3 env = texture(envSampler, r.direction).rgb;
                if (any(isnan(weight)))
                {
                    weight = vec3(0);
                    //hitValue = vec3(1,0,0);
                    //return hitValue;
                }
                else
                    hitValue += env * weight;
            }

            return hitValue;
        }

        /* Hit */
        State state;
        getShadeState(state, prd);
        bool isInside = dot(state.normal, r.direction) > 0.0;
        state.ffnormal = isInside ? -state.normal : state.normal;
        createCoordinateSystem(state.ffnormal, state.tangent, state.bitangent);
        state.eta = isInside ? state.mat.ior : (1.0 / state.mat.ior);
        // return (state.normal + vec3(1)) * 0.5;
        // return vec3(state.mat.roughness);
        // return vec3(state.mat.metallic);
        // return state.mat.f0;
        // return vec3(state.mat.transmission);
        // return state.mat.emission;

        vec3  L = vec3(0); // Vector toward the light
        float lightIntensity = 0.0;
        float lightDistance = 100000.0;
        float lightPdf = 1;

        if (pcRay.dirLightNum + pcRay.pointLightNum > 0) {
            int lightType = 1;
            int lightIdx = int(randFloat(prd.seed, 0, pcRay.dirLightNum + pcRay.pointLightNum));
            if (lightIdx >= pcRay.dirLightNum) {
                lightIdx -= pcRay.dirLightNum;
                lightType = 0;
            }

            // Point light
            if (lightType == 0) {
                vec3 lDir = pointLights[lightIdx].position - state.position;
                lightDistance = length(lDir);
                lightIntensity = pointLights[lightIdx].intensity / (lightDistance * lightDistance * 0.2);
                L = normalize(-lDir);
            }
            else { // Directional light
                lightIntensity = dirLights[lightIdx].intensity;
                L = normalize(dirLights[lightIdx].direction);
            }
        }

        // Reset absorption when ray is going out of surface
        if(!isInside) {
            absorption = vec3(0.0);
        }
        
        // Emission
        hitValue += state.mat.emission * weight;
        
        // Add absoption (transmission / volume)
        // weight *= exp(-absorption * prd.hitT);

        vec3 Li = vec3(0);
        if (state.mat.transmission > 0.0 && isInside || dot(state.ffnormal, L) < 0) {
            BsdfSampleRec directBsdf;
            directBsdf.f = PbrEval(state, -r.direction, state.ffnormal, -L, directBsdf.pdf);
            float misWeightBsdf = max(0, powerHeuristic(directBsdf.pdf, lightPdf)); // multi importance sampling weight
            Li = misWeightBsdf * directBsdf.f * abs(dot(-L, state.ffnormal)) * lightIntensity * weight / directBsdf.pdf;
        }

        BsdfSampleRec bsdf;
        bsdf.f = PbrSample(state, -r.direction, state.ffnormal, bsdf.L, bsdf.pdf, prd.seed);
        // bsdf.f = vec3(0);
        // return bsdf.L * 0.5 + 0.5;

        if (any(isnan(bsdf.f)))
            bsdf.f = vec3(0);
        weight *= bsdf.f * abs(dot(state.ffnormal, bsdf.L)) / bsdf.pdf;

        // Direct light
        if (state.mat.transmission > 0.0 && isInside || dot(state.ffnormal, L) < 0) {
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

vec3 defoucueDiskSample(float focusDist)
{
    vec3 cameraFront = vec3(0,0,1);
    vec3 cameraUp = vec3(0,1,0);
    vec3 cameraRight=vec3(1,0,0);

    float defocusRadius = focusDist * tan(pcRay.defocusAngle/2);

    vec3 defocusDiskU = cameraRight * defocusRadius;
    vec3 defocusDiskV = cameraUp * defocusRadius;

    vec3 p = randonInUnitDisk(prd.seed);

    vec3 defoucueDisk = defocusDiskU * p.x + defocusDiskV * p.y;

    return defoucueDisk;
}

vec3 samplePixel(ivec2 imageCoord, ivec2 imageSize, int maxDepth)
{
    float r1 = rnd(prd.seed);
    float r2 = rnd(prd.seed);
    // Subpixel jitter: send the ray through a different position inside the pixel
    // each time, to provide antialiasing.
    vec2 subpixel_jitter = vec2(r1, r2);

    const vec2 pixelCenter = vec2(imageCoord) + subpixel_jitter;
    const vec2 inUV = pixelCenter / vec2(imageSize);
    vec2 d = inUV * 2.0 - 1.0;

    float focusDistRatio = pcRay.focusDist / pcRay.zFar;

    //vec4 origin = uni.viewInverse * vec4(0, 0, 0, 1);
    vec4 originView = (pcRay.defocusAngle <= 0) ? vec4(0, 0, 0, 1) : ( vec4(defoucueDiskSample(focusDistRatio), 1) );
    vec4 originWorld = uni.viewInverse * originView;
    
    //vec4 target = uni.projInverse * vec4(d.x, d.y, focusDistRatio, 1);
    vec4 targetView = uni.projInverse * vec4(d.x, d.y, -1, 1) * pcRay.focusDist;
    //target.z*=focusDistRatio;
    vec4 direction = uni.viewInverse * vec4(normalize(targetView.xyz-originView.xyz), 0);

    Ray r = Ray(originWorld.xyz, direction.xyz);
    vec3 radiance = pathtrace(r, maxDepth);

    return radiance;
}