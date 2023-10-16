#version 460

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

layout(set = 1, binding = 2) uniform _ShadowUniform {
    ShadowData shadowUniform;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec2 fragCoord;

void main()
{
    int id = constants.objId;
    mat4 model = objectBuffer.objects[id].model;

    gl_Position = shadowUniform.lightSpace * model * vec4(inPosition, 1.0);
    
    fragCoord = inTexCoord;
}