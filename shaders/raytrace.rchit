#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "raycommon.glsl"
#include "wavefront.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(location = 1) rayPayloadEXT bool isShadowed;
hitAttributeEXT vec2 attribs;

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices { ivec3 i[]; }; // Triangle indices
layout(buffer_reference, scalar) buffer Materials { Material m[]; }; // Array of all materials on an object
layout(buffer_reference, scalar) buffer MatIndices { int i[]; }; // Material ID for each triangle

layout(set = 0, binding = eTlas) uniform accelerationStructureEXT topLevelAS;
layout(set = 1, binding = eObjDescs, scalar) buffer ObjDescBuffer { ObjDesc i[]; } objDesc;
layout(set = 1, binding = eTextures) uniform sampler2D[] textureSampler;

layout(push_constant) uniform _PushConstantRay { PushConstantRay pcRay; };

vec3 interpolate(vec3 a, vec3 b, vec3 c, vec3 barycentrics) {
    return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

mat3 calcTBN(vec3 T, vec3 B, vec3 N) {
    T = normalize(vec3(gl_ObjectToWorldEXT * vec4(T, 0.0)));
    B = normalize(vec3(gl_ObjectToWorldEXT * vec4(B, 0.0)));
    N = normalize(vec3(gl_ObjectToWorldEXT * vec4(N, 0.0)));
    return mat3(T, B, N);
}

void main()
{
    // object data
    ObjDesc objResource = objDesc.i[gl_InstanceCustomIndexEXT];
    MatIndices matIndices = MatIndices(objResource.materialIndexAddress);
    Materials materials = Materials(objResource.materialAddress);
    
    Indices indices = Indices(objResource.indexAddress);
    Vertices vertices = Vertices(objResource.vertexAddress);

    int matIndex = matIndices.i[gl_PrimitiveID];
    Material mat = materials.m[matIndex];

    // Indices of the triangle
    ivec3 ind = indices.i[gl_PrimitiveID];
  
    // Vertex of the triangle
    Vertex v0 = vertices.v[ind.x];
    Vertex v1 = vertices.v[ind.y];
    Vertex v2 = vertices.v[ind.z];

    // centroid
    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    // Computing the coordinates of the hit position
    const vec3 pos = v0.pos * barycentrics.x + v1.pos * barycentrics.y + v2.pos * barycentrics.z;
    const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));  // Transforming the position to world space

    // texture coordinate
    const vec2 texCoord = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;
    
    const mat3 TBN = calcTBN(v0.tangent, v0.bitangent, v0.normal) * barycentrics.x + 
        calcTBN(v1.tangent, v1.bitangent, v1.normal) * barycentrics.y + 
        calcTBN(v2.tangent, v2.bitangent, v2.normal) * barycentrics.z;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 nrm;
    if (mat.diff_textureId >= 0) {
        diffuse = vec3(texture(textureSampler[mat.diff_textureId], texCoord));
        ambient = diffuse;
    }
    else {
        diffuse = mat.diffuse;
        ambient = diffuse;
    }

    if (mat.spec_textureId >= 0) {
        specular = vec3(texture(textureSampler[mat.spec_textureId], texCoord));
    }
    else {
        specular = mat.specular;
    }

    if (mat.norm_textureId >= 0) {
        nrm = texture(textureSampler[mat.norm_textureId], texCoord).rgb;
        nrm = normalize(nrm * 2.0 - 1.0);
        nrm = normalize(TBN * nrm);
    }
    else {
        nrm = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
        nrm = normalize(vec3(nrm * gl_WorldToObjectEXT));
    }
    // nrm = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;

    // Computing the normal at hit position
    const vec3 worldNrm = normalize(vec3(nrm * gl_WorldToObjectEXT));  // Transforming the normal to world space

    // Vector toward the light
    vec3  L;
    float lightIntensity = pcRay.lightIntensity;
    float lightDistance = 100000.0;
    // Point light
    if(pcRay.lightType == 0)
    {
        vec3 lDir = pcRay.lightPosition - worldPos;
        lightDistance = length(lDir);
        lightIntensity = pcRay.lightIntensity / (lightDistance * lightDistance);
        L = normalize(lDir);
    }
    else  // Directional light
    {
        L = normalize(pcRay.lightPosition);
    }

    float attenuation = 1;
    diffuse = computeDiffuse(diffuse, ambient * 0.1, L, nrm);

    // Tracing shadow ray only if the light is visible from the surface
    if(dot(nrm, L) > 0)
    {
        float tMin   = 0.001;
        float tMax   = lightDistance;
        vec3  origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
        vec3  rayDir = L;
        uint  flags =
            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
        isShadowed = true;
        traceRayEXT(topLevelAS,  // acceleration structure
                flags,       // rayFlags
                0xFF,        // cullMask
                0,           // sbtRecordOffset
                0,           // sbtRecordStride
                1,           // missIndex
                origin,      // ray origin
                tMin,        // ray min range
                rayDir,      // ray direction
                tMax,        // ray max range
                1            // payload (location = 1)
        );

        if(isShadowed) {
            attenuation = 0.3;
            specular = vec3(0);
        }
        else {
            // Specular
            specular = computeSpecular(specular, mat.shininess, -gl_WorldRayDirectionEXT, L, nrm);
        }
    }
    else {
        specular = vec3(0);
    }

    prd.hitValue = vec3(lightIntensity * attenuation * (diffuse + specular));
}
