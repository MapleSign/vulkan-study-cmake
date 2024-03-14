#version 460

#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "host_device.h"

layout(push_constant) uniform PushConstants {
    PushConstantRaster constants;
};

layout(set = 1, binding = 0) buffer DirLightInfo {
    DirLight dirLight[];
};

layout(triangles) in;
layout(triangle_strip, max_vertices=48) out;

layout(location = 0) in VS_OUT {
    vec2 fragCoord;
} gs_in[];

layout(location = 0) out vec2 fragCoord;

void main() {
    for (int lightIdx = 0; lightIdx < constants.dirLightNum; ++lightIdx) {
        for (int level = 0; level < dirLight[lightIdx].csmLevel; ++level) {
            gl_Layer = lightIdx * MAX_CSM_LEVEL + level;
            for (int i = 0; i < 3; ++i) {
                gl_Position = dirLight[lightIdx].lightSpaces[level] * gl_in[i].gl_Position;
                fragCoord = gs_in[i].fragCoord;
                EmitVertex();
            }
            EndPrimitive();
        }
    }
}