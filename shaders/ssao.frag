#version 450

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "host_device.h"

layout(set = 0, binding = 0) uniform _SSAOUniform {
    SSAOData ssaoUniform;
};

layout(set = 0, binding = 1) uniform sampler2D gPosition;
layout(set = 0, binding = 2) uniform sampler2D gNormal;
layout(set = 0, binding = 3) uniform sampler2D texNoise;

layout(location = 0) in vec2 inUV;

layout(location = 0) out float outOcc;

void main() {
    vec2 noiseScale = ssaoUniform.windowSize / textureSize(texNoise, 0);
    vec3 fragPos = texture(gPosition, inUV).xyz;
    fragPos = vec3(ssaoUniform.view * vec4(fragPos, 1.0));
    vec3 normal = texture(gNormal, inUV).rgb;
    vec3 randomVec = texture(texNoise, inUV * noiseScale).xyz;

    // random TBN
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for(int i = 0; i < ssaoUniform.kernelSize; ++i)
    {
        // get sample position
        vec3 samplePos = TBN * ssaoUniform.samples[i].xyz; // from tangent to view-space
        samplePos = fragPos + samplePos * ssaoUniform.radius;

        vec4 offset = vec4(samplePos, 1.0);
        offset = ssaoUniform.projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz  = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

        float sampleDepth = (ssaoUniform.view * vec4(texture(gPosition, offset.xy).xyz, 1.0)).z;
        float rangeCheck = smoothstep(0.0, 1.0, ssaoUniform.radius / abs(fragPos.z - sampleDepth)); // check depth range
        occlusion += (sampleDepth >= samplePos.z + ssaoUniform.bias ? 1.0 : 0.0) * rangeCheck;    
    }
    occlusion = 1.0 - (occlusion / ssaoUniform.kernelSize);

    outOcc = occlusion;
}