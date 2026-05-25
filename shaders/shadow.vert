#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "host_device.h"

layout(push_constant) uniform PushConstants {
    PushConstantShadow constants;
};

layout(set = 0, binding = eGlobals, scalar) uniform GlobalUnifrom {
    GlobalData global;
} globalUniform;

layout(set = 0, binding = eObjData, scalar) readonly buffer ObjectBuffer {
	ObjectData objects[];
} objectBuffer;

layout(set = 1, binding = 0) readonly buffer DirLightInfo {
    DirLight dirLights[];
};

layout(set = 1, binding = 1) readonly buffer PointLightInfo {
    PointLight pointLights[];
};

layout(set = 1, binding = 2) uniform _ShadowUniform {
    ShadowData shadowUniform;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out VS_OUT {
    vec2 fragCoord;
    vec3 fragPosWS;
};

void main()
{
    int id = constants.objId;
    mat4 model = objectBuffer.objects[id].model;
    vec4 worldPos = model * vec4(inPosition, 1.0);

    mat4 projView = mat4(1.0);
    if (constants.lightType == LIGHT_TYPE_DIR) {
        projView = dirLights[constants.lightId].lightSpaces[constants.layerId];
    } else if (constants.lightType == LIGHT_TYPE_POINT) {
        projView = pointLights[constants.lightId].lightSpaces[constants.layerId];
    }
    gl_Position = projView * worldPos;
    
    fragCoord = inTexCoord;
    fragPosWS = worldPos.xyz;
}