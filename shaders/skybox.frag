#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "host_device.h"

layout(set = 0, binding = eEnvTexture) uniform samplerCube envSampler;

layout(location = 0) in vec3 fragCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = textureLod(envSampler, fragCoord, 0);
}