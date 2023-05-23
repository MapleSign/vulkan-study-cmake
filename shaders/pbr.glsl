float powerHeuristic(float a, float b)
{
    float t = a * a;
    return t / (b * b + t);
}

vec3 CosineSampleHemisphere(float r1, float r2)
{
    vec3  dir;
    float r = sqrt(r1);
    float phi = 2 * M_PI * r2;
    dir.x = r * cos(phi);
    dir.y = r * sin(phi);
    dir.z = sqrt(max(0.0, 1.0 - dir.x * dir.x - dir.y * dir.y));

    return dir;
}

vec3 SphericalDirection(float sinTheta, float cosTheta, float phi) {
    return vec3(sinTheta * cos(phi), 
                sinTheta * sin(phi),
                cosTheta);
}

float GgxD(float alpha, float NdotH) {
    float alpha2 = alpha * alpha;
    float f = NdotH * NdotH * (alpha2 - 1) + 1;
    return alpha2 / (M_PI * f * f);

    // float tan2Theta = Tan2Theta(wh);
    // if (isinf(tan2Theta))
    //     return 0;
    // const Float cos4Theta = Cos2Theta(wh) * Cos2Theta(wh);
    // float e = (Cos2Phi(wh) / (alphax * alphax) + Sin2Phi(wh) / (alphay * alphay)) * tan2Theta;
    // return 1 / (Pi * alphax * alphay * cos4Theta * (1 + e) * (1 + e));
}

vec3 GgxSample(float alpha, float r1, float r2) {
    float tanTheta2 = alpha * alpha * r1 / (1.0f - r1);
    float cosTheta = 1 / sqrt(1 + tanTheta2);
    float phi = (2 * M_PI) * r2;

    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    return SphericalDirection(sinTheta, cosTheta, phi);
}

vec3 GgxEval(vec3 f0, float alpha, float roughness, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf) {
    float NdotH = clamp(dot(N, H), 0, 1);
    float LdotH = clamp(dot(L, H), 0, 1);
    float D = GgxD(alpha, NdotH);

    pdf = D * NdotH / (4 * LdotH);

    float NdotL = clamp(dot(N, L), 0, 1);
    float NdotV = clamp(dot(N, V), 0, 1);
    float k = alpha / 2;
    float G1_L = NdotL / (NdotL * (1 - k) + k);
    float G1_V = NdotV / (NdotV * (1 - k) + k);
    float G = G1_L * G1_V;

    float VdotH = clamp(dot(V, H), 0, 1);
    vec3 F = f0 + (vec3(1) - f0) * pow(1 - abs(VdotH), 5);
    return D * F * G / (4 * NdotL * NdotV);
}

// refer to pbrt & "Real Shading in Unreal Engine 4"
vec3 PbrSample(inout State state, vec3 V, vec3 N, inout vec3 L, inout float pdf, inout uint seed) {
    vec3 brdf = vec3(0);

    float r1 = rnd(seed);
    float r2 = rnd(seed);

    float diffuseRatio = (1.0 - state.mat.metallic);
    float specularRatio = 1 - diffuseRatio;
    // light is diffuse reflect
    if (rnd(seed) < diffuseRatio) {
        // Lambertien reflect
        L = CosineSampleHemisphere(r1, r2); // sample diffuse light direction
        L = state.tangent * L.x + state.bitangent * L.y + N * L.z; // map to world normal

        brdf = state.mat.albedo / M_PI * (1.0 - state.mat.metallic);
        pdf = dot(L, N) / M_PI;
    }
    else { // light is specular reflect
        // Cook-Torrance microfacet specular
        float roughness = state.mat.roughness;
        float alpha = roughness * roughness;
        vec3 H = GgxSample(alpha, r1, r2); // sample wh
        H = state.tangent * H.x + state.bitangent * H.y + N * H.z; // map to world normal
        if (dot(H, N) < 0)
            H = -H;
        L = reflect(-V, H);

        brdf = GgxEval(state.mat.f0, alpha, roughness, V, N, L, H, pdf);
        // pdf *= specularRatio;
    }

    return brdf;
}

vec3 PbrEval(inout State state, vec3 V, vec3 N, vec3 L, inout float pdf) {
    vec3 H = normalize(L + V);
    if(dot(N, H) < 0.0)
        H = -H;
    
    float diffuseRatio = (1.0 - state.mat.metallic);
    float specularRatio = 1 - diffuseRatio;

    vec3 brdf = vec3(0);
    pdf = 0;

    brdf += state.mat.albedo / M_PI * (1.0 - state.mat.metallic);
    pdf += dot(L, N) / M_PI;

    float roughness = state.mat.roughness;
    float alpha = roughness * roughness;
    float specPdf;
    brdf += GgxEval(state.mat.f0, alpha, roughness, V, N, L, H, specPdf);
    pdf += specPdf;

    return brdf;
}
