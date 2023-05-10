struct hitPayload
{
    vec3 hitValue;
};

struct Material
{
    vec3  albedo;
    float specular;
    vec3  emission;
    float metallic;
    float roughness;
    float clearcoat;
    float clearcoatRoughness;
    float transmission;

    vec3 f0;
    float ior;
    float alpha;
};

struct State {
    float eta;
    
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 texCoord;
    vec3 ffnormal;

    uint matId;
    Material mat;
};
