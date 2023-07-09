#version 450
layout(location = 0) in vec2 outUV;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D colorTxt;
layout(set = 0, binding = 1) uniform sampler2D depthTxt;
layout(set = 0, binding = 2) uniform GlobalUnifrom {
    mat4 view;
    mat4 proj;
    mat4 viewInverse;
    mat4 projInverse;
};

layout(push_constant) uniform shaderInformation
{
    float aspectRatio;
}
pushc;

vec2 barrel_distort(vec2 uv) {
    float distort = 0.5;
    float adjust = 1.0 / (0.75 + 0.5 * distort);

    vec2 d = uv - vec2(0.5);
    float d2 = dot(d, d);
    float f = 1.0 + d2 * distort;
    return f * adjust * d + vec2(0.5);
}

void main()
{
    vec4 fragPos = vec4((outUV * 2.0) - 1.0, 1.0, 1.0);
    vec4 newfragPos = proj * view * viewInverse * projInverse * fragPos;
    vec2 uv = (newfragPos.xy + 1.0) * 0.5;
    uv = barrel_distort(uv);
    // vec2 uv = barrel_distort(outUV);
    float gamma = 1. / 2.2;
    if (uv.x > 1 || uv.x < 0 || uv.y > 1 || uv.y < 0)
        fragColor = vec4(vec3(0.0), 1.0);
    else
        fragColor = texture(colorTxt, uv);
}
