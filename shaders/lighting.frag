#version 460

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#extension GL_ARB_shader_clock : enable

#include "raycommon.glsl"
#include "random.glsl"
#include "host_device.h"

layout(push_constant) uniform PushConstants {
    PushConstantRaster constants;
};

layout(set = 0, binding = eEnvTexture) uniform samplerCube envSampler;

layout(set = 1, binding = 0) buffer DirLightInfo {
    DirLight dirLight[];
};

layout(set = 1, binding = 1) buffer PointLightInfo {
    PointLight pointLights[];
};

layout(set = 1, binding = 2) uniform _ShadowUniform {
    ShadowData shadowUniform;
};

layout(set = 1, binding = 3) uniform sampler2D[] dirLightShadowMaps;
layout(set = 1, binding = 4) uniform samplerCube[] pointLightShadowMaps;

layout(input_attachment_index = eSceneColor, set = 2, binding = eSceneColor) uniform subpassInput inputSceneColor;
layout(input_attachment_index = ePosition, set = 2, binding = ePosition) uniform subpassInput inputPos;
layout(input_attachment_index = eNormal, set = 2, binding = eNormal) uniform subpassInput inputNormal;
layout(input_attachment_index = eAlbedo, set = 2, binding = eAlbedo) uniform subpassInput inputAlbedo;
layout(input_attachment_index = eMetalRough, set = 2, binding = eMetalRough) uniform subpassInput inputMetalRough;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

#include "pbr.glsl"

vec3 calcLight(inout State state, vec3 V, vec3 L, vec3 lightIntensity, float lightPdf) {
    vec3 Li = vec3(0);
    if (dot(state.ffnormal, L) > 0) {
        BsdfSampleRec directBsdf;
        directBsdf.f = PbrEval(state, V, state.ffnormal, L, directBsdf.pdf);
        float misWeightBsdf = max(0, powerHeuristic(directBsdf.pdf, lightPdf)); // multi importance sampling weight
        Li = misWeightBsdf * directBsdf.f * abs(dot(L, state.ffnormal)) * lightIntensity / directBsdf.pdf;
    }
    return Li;
}

float findBlocker(sampler2D shadowMap, vec2 projCoords, float projDepth) {
    int size = 2;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    int cnt = 0;
    float blockerDepth = 0.0;
    for (int x = -size; x <= size; ++x) {
        for (int y = -size; y <= size; ++y) {
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            if (projDepth > closestDepth) {
                blockerDepth += closestDepth;
                cnt += 1;
            }
        }
    }

    if (cnt == 0)
        blockerDepth = 1.0;
    else
        blockerDepth /= cnt;
    
    return blockerDepth;
}

float PCF(sampler2D shadowMap, int flterSize, vec2 projCoords, float projDepth) {
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    float shadow = 0.0;
    for(int x = -flterSize; x <= flterSize; ++x)
    {
        for(int y = -flterSize; y <= flterSize; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projDepth > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= (flterSize * 2 + 1) * (flterSize * 2 + 1);

    return shadow;
}

float calcDirShadow(sampler2D shadowMap, vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z - shadowUniform.bias;
    float shadow = 0.0;
    
    if (shadowUniform.type == 0) {
        shadow = currentDepth > closestDepth ? 1.0 : 0.0;
    }
    else if (shadowUniform.type == 1) {
        shadow = PCF(shadowMap, shadowUniform.pcfFilterSize, projCoords.xy, currentDepth);
    }
    else {
        float lightWidth = 50.0;
        float blockerDepth = findBlocker(shadowMap, projCoords.xy, currentDepth);
        float penumbraSize = max(currentDepth - blockerDepth, 0.0) / blockerDepth * lightWidth;
        shadow = PCF(shadowMap, int(penumbraSize/2), projCoords.xy, currentDepth);
    }

    return shadow;
}

const vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float calcCubeShadow(samplerCube shadowMap, vec3 fragPos, vec3 lightPos, vec3 viewPos) {
    int samples = 20;
    float farPlane = 25.0;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / farPlane)) / 500.0;

    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight) - shadowUniform.bias;
    float shadow = 0.0;

    if (shadowUniform.type == 0) {
        float closestDepth = texture(shadowMap, fragToLight * diskRadius).r;
        closestDepth *= farPlane;
        shadow += currentDepth > closestDepth ? 1.0 : 0.0;
    }
    else {
        for (int i = 0; i < samples; ++i) {
            float closestDepth = texture(shadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
            closestDepth *= farPlane;
            shadow += currentDepth > closestDepth ? 1.0 : 0.0;
        }
        shadow /= float(samples);
    }

    return shadow;
}

void main() {
    if (subpassLoad(inputSceneColor).a < 1.0)
        discard;
    
    vec3 fragPos = subpassLoad(inputPos).rgb;
    vec3 viewDir = normalize(constants.viewPos - fragPos);

    State state;
    state.position = fragPos;
    state.normal = (subpassLoad(inputNormal).rgb - 0.5) * 2.0;
    state.ffnormal = dot(state.normal, viewDir) >= 0.0 ? state.normal : -state.normal;
    createCoordinateSystem(state.ffnormal, state.tangent, state.bitangent);
    
    state.mat.albedo = subpassLoad(inputAlbedo).rgb;
    state.mat.metallic = subpassLoad(inputMetalRough).r;
    state.mat.roughness = subpassLoad(inputMetalRough).g;
    state.mat.ior = 1.4;
    state.mat.emission = subpassLoad(inputSceneColor).rgb;
    state.mat.clearcoat = 0.0;
    state.mat.transmission = 0.0;
    
    vec3 result = vec3(0.0);
    for (int i = 0; i < constants.dirLightNum; ++i) {
        vec3 lightDir = -normalize(dirLight[i].direction);
        vec3 lightIntensity = dirLight[i].intensity * dirLight[i].color;

        vec4 fragPosLightSpace = dirLight[i].lightSpace * vec4(fragPos, 1.0);
        float shadow = i < shadowUniform.maxDirShadowNum ? calcDirShadow(dirLightShadowMaps[nonuniformEXT(i)], fragPosLightSpace) : 0.0;

        result += (1.0 - shadow) * calcLight(state, viewDir, lightDir, lightIntensity, 1.0);
    }
    
    for (int i = 0; i < constants.pointLightNum; ++i) {
        PointLight light = pointLights[i];
        vec3 lightDir = normalize(light.position - fragPos);
        float distance = length(light.position - fragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
        vec3 lightIntensity = light.color * light.intensity * attenuation;
        
        float shadow = i < shadowUniform.maxPointShadowNum ? 
            calcCubeShadow(pointLightShadowMaps[nonuniformEXT(i)], fragPos, light.position, constants.viewPos) : 0.0;

        result += (1.0 - shadow) * calcLight(state, viewDir, lightDir, lightIntensity, 1.0);
    }

    int numSamples = 64;
    BsdfSampleRec indirectBsdf;
    uint seed = tea(int(gl_FragCoord.y + gl_FragCoord.x), int(clockARB()));
    vec3 sampleColor = vec3(0);
    for (int i = 0; i < numSamples; ++i) {
        indirectBsdf.f = PbrSample(state, viewDir, state.ffnormal, indirectBsdf.L, indirectBsdf.pdf, seed);
        vec3 sampleLight = texture(envSampler, indirectBsdf.L).rgb;
        vec3 indirectSample = indirectBsdf.f * sampleLight * abs(dot(state.ffnormal, indirectBsdf.L)) / indirectBsdf.pdf;
        if (any(isnan(indirectSample)))
            sampleColor += vec3(0.0);
        else
            sampleColor += indirectSample;
    }
    sampleColor /= numSamples;
    result += sampleColor;

    outColor = vec4(result + state.mat.emission, 1.0);
    // outColor = vec4(vec3(shadow), 1.0);
    // outColor = vec4((state.normal + vec3(1)) * 0.5, 1.0);
    // outColor = vec4(state.texCoord, 0, 1);
    // outColor = vec4(1.0);
}