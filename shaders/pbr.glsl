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

float FrDielectric(float cosTheta_i, float eta) {
    float sinTheta2_i = 1 - cosTheta_i * cosTheta_i;
    float sinTheta2_t = sinTheta2_i / (eta * eta);
    if (sinTheta2_t >= 1.0)
        return 1.0;
    float cosTheta_t = sqrt(1 - sinTheta2_t);

    float r_parl = (eta * cosTheta_i - cosTheta_t) / 
        (eta * cosTheta_i + cosTheta_t);
    float r_perp = (cosTheta_i - eta * cosTheta_t) / 
        (cosTheta_i + eta * cosTheta_t);
    return (r_parl * r_parl + r_perp * r_perp) / 2;
}

float Lambda(float alpha, float cosTheta) {
    float cosTheta2 = cosTheta * cosTheta;
    float tanTheta2 = (1.0 - cosTheta2) / cosTheta2;
    return (sqrt(1 + alpha * alpha * tanTheta2) - 1) * 0.5;
}

float GgxD(float alpha, float NdotH) {
    float alpha2 = alpha * alpha;
    float f = NdotH * NdotH * (alpha2 - 1) + 1;
    return alpha2 / (M_PI * f * f);
}

float GgxD_w(float alpha, float NdotV, float NdotH, float VdotH) {
    float G1 = 1 / (1 + Lambda(alpha, NdotV));
    float D = GgxD(alpha, NdotH);
    return G1 / abs(NdotV) * D * abs(VdotH);
}

float GgxG(float alpha, float NdotL, float NdotV) {
    float Lambda_L = Lambda(alpha, NdotL);
    float Lambda_V = Lambda(alpha, NdotV);
    return 1 / (1 + Lambda_L + Lambda_V);
}

// V = G / (4 * NdotL * NdotV)
float GgxV(float alpha, float NdotL, float NdotV) {
    float Lambda_L = Lambda(alpha, NdotL);
    float Lambda_V = Lambda(alpha, NdotV);
    float G = GgxG(alpha, NdotL, NdotV);
    
    return G / (4 * NdotL * NdotV);
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

float GgxEvalDielectricReflect(float Fr, float alpha, float roughness, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf) {
    float NdotH = clamp(dot(N, H), 0, 1);
    float LdotH = clamp(dot(L, H), 0, 1);
    float D = GgxD(alpha, NdotH);

    float VdotH = dot(V, H);
    float NdotL = clamp(dot(N, L), 0, 1);
    float NdotV = clamp(dot(N, V), 0, 1);

    pdf = GgxD_w(alpha, NdotV, NdotH, VdotH) / (4 * abs(VdotH));
    
    float GGX_V = GgxV(alpha, NdotL, NdotV);

    return D * Fr * GGX_V;
}

float GgxEvalDielectricRefract(float Ft, float alpha, float roughness, float eta, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf) {
    float NdotH = dot(N, H);
    float LdotH = dot(L, H);
    float D = GgxD(alpha, NdotH);

    float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    
    float G = GgxG(alpha, NdotL, NdotV);
    
    float VdotH = dot(V, H);
    float denom = VdotH * eta + LdotH;
    denom *= denom;

    float dwm_dwi = abs(LdotH) / denom;
    pdf = GgxD_w(alpha, NdotV, NdotH, VdotH) * dwm_dwi;

    return Ft * abs(D * G * LdotH * VdotH / (denom * NdotL * NdotV));
}

// refer to pbrt & "Real Shading in Unreal Engine 4"
vec3 PbrSample(inout State state, vec3 V, vec3 N, inout vec3 L, inout float pdf, inout uint seed) {
    vec3 brdf = vec3(0);

    float r1 = rnd(seed);
    float r2 = rnd(seed);

    float diffuseRatio = (1.0 - state.mat.metallic);
    float specularRatio = 1 - diffuseRatio;
    float transmissionWeight = state.mat.transmission; // only dielectric will transmission

    float roughness = state.mat.roughness;
    float alpha = max(roughness * roughness, 0.001);
    float eta = state.eta;
    vec3 H;

    if (rnd(seed) < state.mat.clearcoat) {
        float dielectricSpecular = (state.mat.ior - 1) / (state.mat.ior + 1);
        dielectricSpecular *= dielectricSpecular;
        vec3 clearcoat_f0 = vec3(dielectricSpecular);

        roughness = state.mat.clearcoatRoughness;
        alpha = max(roughness * roughness, 0.001);

        H = GgxSample(alpha, r1, r2); // sample wh
        H = state.tangent * H.x + state.bitangent * H.y + N * H.z; // map to world normal
        if (dot(H, N) < 0)
            H = -H;
        
        L = normalize(reflect(-V, H));
        brdf = GgxEvalReflect(clearcoat_f0, alpha, roughness, V, N, L, H, pdf);
    }
    // light is diffuse reflect or refract -- pure refract or total internal reflect
    else {
        H = GgxSample(alpha, r1, r2); // sample wh
        H = state.tangent * H.x + state.bitangent * H.y + N * H.z; // map to world normal
        if (dot(H, N) < 0)
            H = -H;

        if (rnd(seed) < diffuseRatio) {
            if (rnd(seed) < transmissionWeight) { // refract
                float VdotH = dot(V, H);
                float Fr = FrDielectric(VdotH, eta);

                if (rnd(seed) < Fr) { // total internal reflect or reflect
                    L = normalize(reflect(-V, H));
                    brdf = state.mat.albedo * GgxEvalDielectricReflect(Fr, alpha, roughness, V, N, L, H, pdf);
                    pdf *= Fr;
                }
                else { // refract
                    L = normalize(refract(-V, H, eta));
                    brdf = state.mat.albedo * GgxEvalDielectricRefract(1.0 - Fr, alpha, roughness, eta, V, N, L, H, pdf);
                    pdf *- 1 - Fr;
                    // brdf = state.mat.albedo;
                    // pdf = abs(dot(N, L));
                }
                // brdf = state.mat.albedo;
                // pdf = abs(dot(N, L));
            }
            else { // Lambertien reflect
                L = CosineSampleHemisphere(r1, r2); // sample diffuse light direction
                L = state.tangent * L.x + state.bitangent * L.y + N * L.z; // map to world normal

                brdf = state.mat.albedo / M_PI * (1.0 - state.mat.metallic);
                pdf = dot(L, N) / M_PI;
            }
        }
        else { // light is specular reflect
            // Cook-Torrance microfacet specular
            L = normalize(reflect(-V, H));
            brdf = GgxEvalReflect(state.mat.albedo, alpha, roughness, V, N, L, H, pdf);
            // pdf *= specularRatio;
        }
    }

    return brdf;
}

vec3 PbrEval(inout State state, vec3 V, vec3 N, vec3 L, inout float pdf) {
    vec3 H = normalize(L + V);
    if(dot(N, H) < 0.0)
        H = -H;
    
    float diffuseRatio = (1.0 - state.mat.metallic);
    float specularRatio = 1 - diffuseRatio;
    float transmissionWeight = (1.0 - state.mat.metallic) * state.mat.transmission; // only dielectric will transmission

    float roughness = state.mat.roughness;
    float alpha = max(roughness * roughness, 0.001);

    vec3 bsdf = vec3(0);
    float bsdfPdf = 1;
    vec3 brdf = vec3(0);
    float brdfPdf = 0;

    pdf = 0;

    float NdotL = dot(N, L);
    if (transmissionWeight > 0.0) {
        float eta = state.eta;
        if (NdotL > 0.0) { // reflect
            float Fr = FrDielectric(dot(V, H), eta);
            bsdf = state.mat.albedo * GgxEvalDielectricReflect(Fr, alpha, roughness, V, N, L, H, bsdfPdf);
            bsdfPdf *= Fr;
            // if (all(lessThan(bsdf, vec3(0.000001))) && all(greaterThan(bsdf, vec3(-0.000001))) || bsdfPdf == 0.0) {
            //     bsdf = vec3(0.0, 100.0, 0.0);
            //     bsdfPdf = 1.0;
            // }
        }
        else { // refract
            vec3 refractH = normalize(L + V * eta);
            if(dot(N, refractH) < 0.0)
                refractH = -refractH;
            float Fr = FrDielectric(dot(V, refractH), eta);
            bsdf = state.mat.albedo * GgxEvalDielectricRefract(1 - Fr, alpha, roughness, eta, V, N, L, refractH, bsdfPdf);
            bsdfPdf *= 1 - Fr;
            // if (all(lessThan(bsdf, vec3(0.000001))) && all(greaterThan(bsdf, vec3(-0.000001))) || bsdfPdf > 0.9) {
            //     bsdf = vec3(100.0, 0.0, 0.0);
            //     bsdfPdf = 1.0;
            // }
        }
    }

    if (transmissionWeight < 1.0 && NdotL > 0.0) {
        brdf += state.mat.albedo / M_PI * (1.0 - state.mat.metallic);
        brdfPdf += dot(L, N) / M_PI;

        float specPdf;
        brdf += GgxEvalReflect(state.mat.albedo, alpha, roughness, V, N, L, H, specPdf);
        brdfPdf += specPdf;

        if (state.mat.clearcoat > 0) {
            float dielectricSpecular = (state.mat.ior - 1) / (state.mat.ior + 1);
            dielectricSpecular *= dielectricSpecular;
            vec3 clearcoat_f0 = vec3(dielectricSpecular);

            float clearcoatRoughness = state.mat.clearcoatRoughness;
            float clearcoatAlpha = max(clearcoatRoughness * clearcoatRoughness, 0.001);

            float clearcoatPdf;
            brdf += GgxEvalReflect(clearcoat_f0, clearcoatAlpha, clearcoatRoughness, V, N, L, H, clearcoatPdf);
            brdfPdf += clearcoatPdf;
        }
    }

    pdf = mix(brdfPdf, bsdfPdf, transmissionWeight);

    return mix(brdf, bsdf, transmissionWeight);
}
