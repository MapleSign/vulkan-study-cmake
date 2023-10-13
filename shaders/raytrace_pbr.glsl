#include "pbr.glsl"

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
                float discriminat = 1.0 - eta * eta * (1.0 - VdotH * VdotH);  // total internal reflection
                if (discriminat > 0) { // refract
                    L = normalize(refract(-V, H, eta));
                }
                else { // total internal reflect
                    L = normalize(reflect(-V, H));
                }
                brdf = state.mat.albedo;
                pdf = abs(dot(N, L));
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

    vec3 bsdf = vec3(0);
    float bsdfPdf = 1;
    vec3 brdf = vec3(0);
    float brdfPdf = 0;

    pdf = 0;

    if (transmissionWeight < 1) {
        brdf += state.mat.albedo / M_PI * (1.0 - state.mat.metallic);
        brdfPdf += dot(L, N) / M_PI;

        float roughness = state.mat.roughness;
        float alpha = max(roughness * roughness, 0.001);
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
