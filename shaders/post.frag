#version 450

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_ARB_shader_clock : enable

#include "host_device.h"

layout(location = 0) in vec2 outUV;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D noisyTxt;

layout(push_constant) uniform _PushConstantPost { PushConstantPost pcPost; };

void insertionSort(inout float v[9])
{
  for(int i = 1; i < 9; i++)
  {
    float key = v[i];
    int j = i - 1;
    while(j>=0 && key < v[j])
    {
      v[j+1] = v[j];
      j--;
    }
    v[j+1]=key;
  }
}

float toGray(vec4 color)
{
  return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
}

void main()
{
  vec2  uv    = outUV;
  float gamma = 1. / 2.2;


  ivec2 imageSize = textureSize(noisyTxt,0);
  float offsetX = 1.0 / imageSize.x;
  float offsetY = 1.0 / imageSize.y;


  vec2 offsets[9] = vec2[](
    vec2(-offsetX,  offsetY), // top-left
    vec2( 0.0f,    offsetY), // top-center
    vec2( offsetX,  offsetY), // top-right
    vec2(-offsetX,  0.0f),   // center-left
    vec2( 0.0f,    0.0f),   // center-center
    vec2( offsetX,  0.0f),   // center-right
    vec2(-offsetX, -offsetY), // bottom-left
    vec2( 0.0f,   -offsetY), // bottom-center
    vec2( offsetX, -offsetY)  // bottom-right    
  );

  if(pcPost.enable==0)
  {
    fragColor = texture(noisyTxt, uv);
  }
  else {
    // Mean filter
    if(pcPost.denoisingType == 0)
    {
      float kernel[9] = float[](
        1.0 / 9, 1.0 / 9, 1.0 / 9,
        1.0 / 9, 1.0 / 9, 1.0 / 9,
        1.0 / 9, 1.0 / 9, 1.0 / 9
      );

      vec3 sampleTex[9];
      for(int i = 0; i < 9; i++)
      {
        sampleTex[i] = vec3(texture(noisyTxt,uv+offsets[i]));
      }
      vec3 col = vec3(0.0);
      for(int i = 0;i < 9; i++)
      {
        col += sampleTex[i] * kernel[i];
      }

      fragColor = vec4(col,1.0);
    }
    // Median filter
    else if(pcPost.denoisingType == 1) 
    {
      vec3  sampleTex;
      float sampleR[9];
      float sampleG[9];
      float sampleB[9];

      for(int i = 0; i < 9; i++)
      {
        sampleTex = vec3(texture(noisyTxt,uv+offsets[i]));
        sampleR[i] = sampleTex.x;
        sampleG[i] = sampleTex.y;
        sampleB[i] = sampleTex.z;
      }

      insertionSort(sampleR);
      insertionSort(sampleG);
      insertionSort(sampleB);

      fragColor = vec4(sampleR[4],sampleG[4],sampleB[4],1.0);
    }
    // Bilateral filter
    else if(pcPost.denoisingType == 2)
    {
      const float EPS = 1e-5;
      
      float sigS = max(EPS, pcPost.sigmaSpace);
      float sigC = max(EPS, pcPost.sigmaColor);

      float facS = -1. / (2. * sigS);
      float facC = -1. / (2. * sigC);

      float sumW = 0.;
      vec4  sumC = vec4(0.);
      float halfSize = sigS * 2;
      
      float c = toGray(texture(noisyTxt,uv));

      for(float i = -halfSize; i <= halfSize; i++){
        for(float j = -halfSize; j<= halfSize; j++){
          vec2 pos = vec2(i, j);
          vec4 offsetColor = texture(noisyTxt, uv + pos / imageSize);

          float distS = length(pos);
          float distC = toGray(offsetColor) - c;

          float wS = exp(facS * float(distS * distS));
          float wC = exp(facC * float(distC * distC));
          float w = wS * wC;

          sumW += w;
          sumC += offsetColor * w;
        }
      }

      fragColor = sumC / sumW;

    }
  }
 
}
