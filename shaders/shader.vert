#version 450

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "host_device.h"

layout(push_constant) uniform PushConstants {
    PushConstantRaster constants;
};

layout(set = 0, binding = eGlobals, scalar) uniform GlobalUnifrom {
    GlobalData global;
} globalUniform;

layout(set = 0, binding = eObjData, scalar) readonly buffer ObjectBuffer {
	ObjectData objects[];
} objectBuffer;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBitangent;
layout(location = 5) out mat3 fragTBN; // assigned multiple location 5,6,7

void main() {
    int id = constants.objId;
    mat4 model = objectBuffer.objects[id].model;

    gl_Position = globalUniform.global.proj * globalUniform.global.view * model * vec4(inPosition, 1.0);
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    fragNormal = normalMatrix * inNormal;
    fragTexCoord = inTexCoord;
    fragPos = (model * vec4(inPosition, 1.0)).rgb;

    vec3 T = normalMatrix * inTangent;
    vec3 B = normalMatrix * inBitangent;
    mat3 TBN = mat3(T, B, fragNormal);
    fragTBN = TBN;
    fragTangent = T;
    fragBitangent = B;
}