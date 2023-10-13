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

vec3 F_Schlick(vec3 f0, float VdotH) {
    float reflectance = max(max(f0.r, f0.g), f0.b);
    vec3 f90 = vec3(clamp(reflectance * 50.0, 0.0, 1.0));
    return f0 + (vec3(f90) - f0) * pow(1 - abs(VdotH), 5);
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

// V = G / (4 * NdotL * NdotV)
float GgxV(float alpha, float NdotL, float NdotV) {
    float k = alpha / 2;
    float G1_L = NdotL / (NdotL * (1 - k) + k);
    float G1_V = NdotV / (NdotV * (1 - k) + k);
    float G = G1_L * G1_V;
    
    if (G > 0.0)
        return G / (4 * NdotL * NdotV);
    else return 0.0;
}

vec3 GgxSample(float alpha, float r1, float r2) {
    float tanTheta2 = alpha * alpha * r1 / (1.0f - r1);
    float cosTheta = 1 / sqrt(1 + tanTheta2);
    float phi = (2 * M_PI) * r2;

    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    return SphericalDirection(sinTheta, cosTheta, phi);
}

vec3 GgxEvalReflect(vec3 f0, float alpha, float roughness, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf) {
    float NdotH = clamp(dot(N, H), 0, 1);
    float LdotH = clamp(dot(L, H), 0, 1);
    float D = GgxD(alpha, NdotH);

    pdf = D * NdotH / (4 * LdotH);

    float NdotL = clamp(dot(N, L), 0, 1);
    float NdotV = clamp(dot(N, V), 0, 1);

    float GGX_V = GgxV(alpha, NdotL, NdotV);

    float VdotH = clamp(dot(V, H), 0, 1);
    vec3 F = F_Schlick(f0, VdotH);
    return D * F * GGX_V;
}

vec3 GgxEvalRefract(vec3 f0, float alpha, float roughness, float eta, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf) {
    float NdotH = clamp(dot(N, H), 0, 1);
    float LdotH = clamp(dot(L, H), 0, 1);
    float D = GgxD(alpha, NdotH);

    pdf = D * NdotH / (4 * LdotH);

    float NdotL = clamp(dot(N, L), 0, 1);
    float NdotV = clamp(dot(N, V), 0, 1);
    
    float GGX_V = GgxV(alpha, NdotL, NdotV);
    
    float VdotH = clamp(dot(V, H), 0, 1);
    float sqrtDenom = VdotH + eta * LdotH;
    float factor = 1 / eta;
    vec3 F = F_Schlick(f0, VdotH);
    return (vec3(1.f) - F) *
                abs(D * GGX_V * 4 * eta * eta * LdotH * VdotH * factor * factor /
                        (sqrtDenom * sqrtDenom));
}
