#ifndef GLTFMATERIAL_GLSL
#define GLTFMATERIAL_GLSL 1

void getMetallicRoughness(inout State state, in GltfMaterial material)
{
    // KHR_materials_ior
    float dielectricSpecular = (material.ior - 1) / (material.ior + 1);
    dielectricSpecular *= dielectricSpecular;

    float perceptualRoughness = 0.0;
    float metallic            = 0.0;
    vec4  baseColor           = vec4(0.0, 0.0, 0.0, 1.0);
    vec3  f0                  = vec3(dielectricSpecular);

    // Metallic and Roughness material properties are packed together
    // In glTF, these factors can be specified by fixed scalar values
    // or from a metallic-roughness map
    perceptualRoughness = material.pbrRoughnessFactor;
    metallic = material.pbrMetallicFactor;
    if(material.pbrMetallicRoughnessTexture > -1)
    {
        // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
        // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
        vec4 mrSample = texture(textureSampler[material.pbrMetallicRoughnessTexture], state.texCoord);
        perceptualRoughness = mrSample.g * perceptualRoughness;
        metallic = mrSample.b * metallic;
    }

    // The albedo may be defined from a base texture or a flat color
    baseColor = material.pbrBaseColorFactor;
    if(material.pbrBaseColorTexture > -1)
    {
        baseColor *= texture(textureSampler[material.pbrBaseColorTexture], state.texCoord);
    }

    // baseColor.rgb = mix(baseColor.rgb * (vec3(1.0) - f0), vec3(0), metallic);
    // Specular color (ior 1.4)
    f0 = mix(vec3(dielectricSpecular), baseColor.xyz, metallic);

    state.mat.albedo    = baseColor.xyz;
    state.mat.metallic  = metallic;
    state.mat.roughness = perceptualRoughness;
    state.mat.f0        = f0;
    state.mat.alpha     = baseColor.a;
}

float getPerceivedBrightness(vec3 vector)
{
    return sqrt(0.299 * vector.r * vector.r + 0.587 * vector.g * vector.g + 0.114 * vector.b * vector.b);
}

const float c_MinReflectance = 0.04;
float solveMetallic(vec3 diffuse, vec3 specular, float oneMinusSpecularStrength)
{
    float specularBrightness = getPerceivedBrightness(specular);

    if(specularBrightness < c_MinReflectance)
    {
        return 0.0;
    }

    float diffuseBrightness = getPerceivedBrightness(diffuse);

    float a = c_MinReflectance;
    float b = diffuseBrightness * oneMinusSpecularStrength / (1.0 - c_MinReflectance) + specularBrightness - 2.0 * c_MinReflectance;
    float c = c_MinReflectance - specularBrightness;
    float D = max(b * b - 4.0 * a * c, 0);

    return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

void getSpecularGlossiness(inout State state, in GltfMaterial material)
{
    float perceptualRoughness = 0.0;
    float metallic = 0.0;
    vec4  baseColor = vec4(0.0, 0.0, 0.0, 1.0);

    vec3 f0 = material.khrSpecularFactor;
    perceptualRoughness = 1.0 - material.khrGlossinessFactor;

    if(material.khrSpecularGlossinessTexture > -1)
    {
        vec4 sgSample = texture(textureSampler[material.khrSpecularGlossinessTexture], state.texCoord);
        perceptualRoughness = 1 - material.khrGlossinessFactor * sgSample.a;  // glossiness to roughness
        f0 *= sgSample.rgb;                                                   // specular
    }

    vec3  specularColor = f0;  // f0 = specular
    float oneMinusSpecularStrength = 1.0 - max(max(f0.r, f0.g), f0.b);

    vec4 diffuseColor = material.khrDiffuseFactor;
    if(material.khrDiffuseTexture > -1)
    diffuseColor *= texture(textureSampler[material.khrDiffuseTexture], state.texCoord);

    baseColor.rgb = diffuseColor.rgb * oneMinusSpecularStrength;
    metallic = solveMetallic(diffuseColor.rgb, specularColor, oneMinusSpecularStrength);

    state.mat.albedo    = baseColor.xyz;
    state.mat.metallic  = metallic;
    state.mat.roughness = perceptualRoughness;
    state.mat.f0        = f0;
    state.mat.alpha     = baseColor.a;
}

void getMaterialsAndTextures(inout State state, inout GltfMaterial material)
{
    state.mat.specular = 0.5;

    mat3 TBN = mat3(state.tangent, state.bitangent, state.normal);

    // Perturbating the normal if a normal map is present
    if(material.normalTexture > -1)
    {
        vec3 normalVector = texture(textureSampler[material.normalTexture], state.texCoord).xyz;
        normalVector = normalize(normalVector * 2.0 - 1.0);
        normalVector *= vec3(material.normalTextureScale, material.normalTextureScale, 1.0);
        state.normal = normalize(TBN * normalVector);
    }

    // Emissive term
    state.mat.emission = material.emissiveFactor;
    if(material.emissiveTexture > -1)
        state.mat.emission *=
            texture(textureSampler[material.emissiveTexture], state.texCoord).rgb;

    // Basic material
    if(material.shadingModel == 0)
        getMetallicRoughness(state, material);
    else
        getSpecularGlossiness(state, material);

    // Clamping roughness
    state.mat.roughness = max(state.mat.roughness, 0.001);


    // KHR_materials_transmission
    state.mat.transmission = material.transmissionFactor;
    if(material.transmissionTexture > -1)
    {
        state.mat.transmission *= texture(textureSampler[material.transmissionTexture], state.texCoord).r;
    }

    // KHR_materials_ior
    state.mat.ior = material.ior;
    state.eta = dot(state.normal, state.ffnormal) > 0.0 ? (1.0 / state.mat.ior) : state.mat.ior;

    //KHR_materials_clearcoat
    state.mat.clearcoat = material.clearcoatFactor;
    state.mat.clearcoatRoughness = material.clearcoatRoughness;
    if(material.clearcoatTexture > -1)
    {
        state.mat.clearcoat *= texture(textureSampler[material.clearcoatTexture], state.texCoord).r;
    }
    if(material.clearcoatRoughnessTexture > -1)
    {
        state.mat.clearcoatRoughness *=
            texture(textureSampler[material.clearcoatRoughnessTexture], state.texCoord).g;
    }
    state.mat.clearcoatRoughness = max(state.mat.clearcoatRoughness, 0.001);
}

void getShadeState(inout State state, hitPayload prd) {
    ObjDesc objResource = objDesc.i[prd.instanceCustomIndex];
    MatIndices matIndices = MatIndices(objResource.materialIndexAddress);
    Materials materials = Materials(objResource.materialAddress);
    
    Indices indices = Indices(objResource.indexAddress);
    Vertices vertices = Vertices(objResource.vertexAddress);

    state.matId = matIndices.i[prd.primitiveID];

    // Indices of the triangle
    ivec3 ind = indices.i[prd.primitiveID];
  
    // Vertex of the triangle
    Vertex v0 = vertices.v[ind.x];
    Vertex v1 = vertices.v[ind.y];
    Vertex v2 = vertices.v[ind.z];

    // centroid
    const vec3 barycentrics = vec3(1.0 - prd.baryCoord.x - prd.baryCoord.y, prd.baryCoord.x, prd.baryCoord.y);
    // Computing the coordinates of the hit position
    const vec3 pos = v0.pos * barycentrics.x + v1.pos * barycentrics.y + v2.pos * barycentrics.z;
    const vec3 worldPos = vec3(prd.objectToWorld * vec4(pos, 1.0));  // Transforming the position to world space
    // Normal
    const vec3 nrm = normalize(v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z);
    const vec3 worldNrm = normalize(vec3(nrm * prd.worldToObject));
    // texture coordinate
    const vec2 texCoord = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;
    // Tangent & Bitangent
    const vec3 tangent = normalize(v0.tangent * barycentrics.x + v1.tangent * barycentrics.y + v2.tangent * barycentrics.z);
    vec3 worldTangent = normalize(vec3(tangent * prd.worldToObject));
    const vec3 bitangent = normalize(v0.bitangent * barycentrics.x + v1.bitangent * barycentrics.y + v2.bitangent * barycentrics.z);
    vec3 worldBitangent = normalize(vec3(bitangent * prd.worldToObject));

    state.position = worldPos;
    state.normal = worldNrm;
    state.texCoord = texCoord;
    state.tangent = worldTangent;
    state.bitangent = worldBitangent;

    getMaterialsAndTextures(state, materials.m[state.matId]);
}

#endif