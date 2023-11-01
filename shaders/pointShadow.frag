#version 460

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "host_device.h"

layout(push_constant) uniform PushConstants {
    PushConstantRaster constants;
};

layout(set = 1, binding = 1) buffer PointLightInfo {
    PointLight pointLights[];
};

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices { ivec3 i[]; }; // Triangle indices
layout(buffer_reference, scalar) buffer Materials { GltfMaterial m[]; }; // Array of all materials on an object
layout(buffer_reference, scalar) buffer MatIndices { int i[]; }; // Material ID for each triangle

layout(set = 0, binding = eObjDescs, scalar) buffer ObjDescBuffer { ObjDesc i[]; } objDesc;

layout(set = 0, binding = eTextures) uniform sampler2D[] textureSampler;

layout(location = 0) in vec2 fragCoord;
layout(location = 1) in vec3 fragPos;

void main()
{
    ObjDesc objResource = objDesc.i[constants.objId];
    MatIndices matIndices = MatIndices(objResource.materialIndexAddress);
    Materials materials = Materials(objResource.materialAddress);

    int matIndex = matIndices.i[gl_PrimitiveID];
    GltfMaterial mat = materials.m[matIndex];

    vec4 baseColor;
    if (mat.shadingModel == 0) {
        baseColor = mat.pbrBaseColorFactor;
        if(mat.pbrBaseColorTexture > -1)
        {
            baseColor *= texture(textureSampler[nonuniformEXT(mat.pbrBaseColorTexture)], fragCoord);
        }
    }
    else {
        baseColor = mat.khrDiffuseFactor;
        if(mat.khrDiffuseTexture > -1)
            baseColor *= texture(textureSampler[nonuniformEXT(mat.khrDiffuseTexture)], fragCoord);
    }

    if(mat.alphaMode == ALPHA_MASK && baseColor.a < mat.alphaCutoff) {
        discard;
    }

    int lightIdx = gl_Layer / 6;
    float lightDistance = length(fragPos - pointLights[lightIdx].position);
    lightDistance /= 25.0;
    gl_FragDepth = lightDistance;
}