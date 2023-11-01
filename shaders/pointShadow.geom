#version 460

#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "host_device.h"

layout(push_constant) uniform PushConstants {
    PushConstantRaster constants;
};

layout(set = 1, binding = 1) buffer PointLightInfo {
    PointLight pointLights[];
};

layout(triangles) in;
layout(triangle_strip, max_vertices=18*4) out;

layout(location = 0) in VS_OUT {
    vec2 fragCoord;
} gs_in[];

layout(location = 0) out vec2 fragCoord;
layout(location = 1) out vec3 fragPos;

void main() {
    for (int lightIdx = 0; lightIdx < constants.dirLightNum; ++lightIdx) {
        for (int face = 0; face < 6; ++face) {
            gl_Layer = lightIdx * 6 + face;
            for (int i = 0; i < 3; ++i) {
                gl_Position = pointLights[lightIdx].lightSpaces[face] * gl_in[i].gl_Position;
                fragCoord = gs_in[i].fragCoord;
                fragPos = gl_in[i].gl_Position.xyz;
                EmitVertex();
            }
            EndPrimitive();
        }
    }
}