#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "host_device.h"

layout(set = 0, binding = eGlobals, scalar) uniform GlobalUnifrom {
    GlobalData global;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec3 fragCoord;

void main() {
    fragCoord = inPosition;
    gl_Position = global.proj * mat4(mat3(global.view)) * vec4(inPosition, 1.0);
}
