#version 460

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "raycommon.glsl"
#include "host_device.h"

layout(push_constant) uniform PushConstants {
    PushConstantRaster constants;
};

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices { ivec3 i[]; }; // Triangle indices
layout(buffer_reference, scalar) buffer Materials { GltfMaterial m[]; }; // Array of all materials on an object
layout(buffer_reference, scalar) buffer MatIndices { int i[]; }; // Material ID for each triangle

layout(set = 0, binding = eObjDescs, scalar) buffer ObjDescBuffer { ObjDesc i[]; } objDesc;

layout(set = 0, binding = eTextures) uniform sampler2D[] textureSampler;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBitangent;

layout(location = eSceneColor) out vec4 outSceneColor;
layout(location = ePosition) out vec3 outPos;
layout(location = eNormal) out vec3 outNormal;
layout(location = eAlbedo) out vec4 outAlbedo;
layout(location = eMetalRough) out vec2 outMetalRough;

#include "gltf_material.glsl"

void main() {
    ObjDesc objResource = objDesc.i[constants.objId];
    MatIndices matIndices = MatIndices(objResource.materialIndexAddress);
    Materials materials = Materials(objResource.materialAddress);

    int matIndex = matIndices.i[gl_PrimitiveID];
    GltfMaterial mat = materials.m[matIndex];

    State state;
    state.position = fragPos;
    state.normal = normalize(fragNormal);
    state.texCoord = fragTexCoord;
    state.tangent = normalize(fragTangent);
    state.bitangent = normalize(fragBitangent);
    getMaterialsAndTextures(state, mat);
    
    if(mat.alphaMode == ALPHA_MASK && state.mat.alpha < mat.alphaCutoff) {
        discard;
    }

    outSceneColor = vec4(state.mat.emission, 1.0);
    outPos = fragPos;
    outNormal = state.normal * 0.5 + 0.5;
    outAlbedo = vec4(state.mat.albedo, state.mat.alpha);
    outMetalRough = vec2(state.mat.metallic, state.mat.roughness);
}
