#version 450
layout(location = 0) in vec2 outUV;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D noisyTxt;

layout(push_constant) uniform shaderInformation
{
    float aspectRatio;
}
pushc;

vec2 barrel_distort(vec2 uv) {
    float distort = 0.5;
    float adjust = 1.0 / (1.0 + 0.5 * distort);

    vec2 d = uv - vec2(0.5);
    float d2 = dot(d, d);
    float f = 1.0 + d2 * distort;
    return f * adjust * d + vec2(0.5);
}

void main()
{
    vec2 uv = barrel_distort(outUV);
    float gamma = 1. / 2.2;
    fragColor = texture(noisyTxt, uv);
}
