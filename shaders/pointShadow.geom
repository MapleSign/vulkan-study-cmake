#version 460

#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "host_device.h"

layout(push_constant) uniform PushConstants {
    PushConstantShadow constants;
};

layout(set = 1, binding = 1) readonly buffer PointLightInfo {
    PointLight pointLights[];
};

layout(triangles) in;
layout(triangle_strip, max_vertices=18*4) out;

struct VS_OUT {
    vec2 fragCoord;
    vec3 fragPosWS;
};

layout(location = 0) in VS_OUT gs_in[];

layout(location = 0) out VS_OUT gs_out;

void main() {
    for (int lightIdx = 0; lightIdx < constants.lightNum; ++lightIdx) {
        for (int face = 0; face < 6; ++face) {
            gl_Layer = lightIdx * 6 + face;
            for (int i = 0; i < 3; ++i) {
                gl_Position = pointLights[lightIdx].lightSpaces[face] * gl_in[i].gl_Position;
                gs_out.fragCoord = gs_in[i].fragCoord;
                gs_out.fragPosWS = gs_in[i].fragPosWS;
                EmitVertex();
            }
            EndPrimitive();
        }
    }
}