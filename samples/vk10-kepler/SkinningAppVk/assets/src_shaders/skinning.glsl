//----------------------------------------------------------------------------------
// File:        vk10-kepler\SkinningAppVk\assets\src_shaders/skinning.glsl
// SDK Version: v3.00 
// Email:       gameworks@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2014-2015, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------------
#GLSL_VS
#version 440 core

// This is a simple 2 bone skinning shader

// Attributes
layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iNormal;
layout(location = 2) in vec4 iWeights; // Contains [boneWeight0, boneIndex0, boneWeight1, boneIndex1]

// Uniforms
layout(binding = 0) uniform Block {
	mat4 ModelViewProjection;
	mat4 Bones[9];            // Our simple skeleton only has 9 bones
	ivec4 RenderMode;
	vec4 LightDir0;
	vec4 LightDir1;
};

layout(location = 0) out vec4 color; 

const int EnableTwoBoneSkinning = 0;     // For RenderMode.x
const int DisableSkinning       = 1;     // For RenderMode.x

const int RenderColor   = 0;             // For RenderMode.y
const int RenderNormal  = 1;             // For RenderMode.y
const int RenderWeights = 2;             // For RenderMode.y

void main()
{
  highp vec3 skinnedPosition;   // Location of vertex after skinning in model space
  vec3       skinnedNormal;     // Normal for vertex after skinning in model space

  // The position of the skinned vertex is a weighted sum of the individual bone matrices applied to the position
  float boneWeight0 = iWeights[0];
  int   boneIndex0  = int(iWeights[1]);
  float boneWeight1 = iWeights[2];
  int   boneIndex1  = int(iWeights[3]);

  // Compute the skinned position and normal
  if(RenderMode.x == EnableTwoBoneSkinning)
  {
    // Render with 2 bone skinning; each vertex can be effected by two bones
    skinnedPosition = (boneWeight0 * (Bones[boneIndex0] * vec4(iPosition, 1.0)) +
                       boneWeight1 * (Bones[boneIndex1] * vec4(iPosition, 1.0))).xyz;

    // The normal of the skinned vertex is a weighted sum of the individual bone matrices applied to the normal.
    // Only the 3x3 rotation portion of the bone matrix is applied to the normal
    skinnedNormal = boneWeight0 * (mat3(Bones[boneIndex0]) * iNormal) +
                    boneWeight1 * (mat3(Bones[boneIndex1]) * iNormal);
    skinnedNormal = normalize(skinnedNormal);
  }
  else
  {
    // Render with 1 bone skinning; only a single bone effects each vertex
    skinnedPosition = (Bones[boneIndex0] * vec4(iPosition, 1.0)).xyz;
    skinnedNormal = mat3(Bones[boneIndex0]) * iNormal;
  }


  // Compute the final position
  gl_Position = ModelViewProjection * vec4(skinnedPosition, 1.0);


  // Compute the vertex color.
  color.w = 1.0;
  if(RenderMode.y == RenderColor)
  {
    // Compute a simple diffuse lit vertex color with two lights
    color.xyz = vec3(0.4, 0.0, 0.0) * dot(skinnedNormal, LightDir0.xyz) + vec3(0.4, 0.2, 0.2);
    color.xyz += vec3(0.4, 0.0, 0.0) * dot(skinnedNormal, LightDir1.xyz);
  }
  else if(RenderMode.y == RenderNormal)
  {
    // Output the skinned normal as the color
    color.xyz = 0.5 * skinnedNormal + vec3(0.5, 0.5, 0.5);
  }
  else if(RenderMode.y == RenderWeights)
  {
    if(fract(float(boneIndex0) / 2.0) == 0.0) // avoiding %
    {
      // Render even bones as primarily red with blue for the secondary bone influence
      color.xyz = vec3(boneWeight0, boneWeight1, 0);
    }
    else
    {
      // Render odd bones as primarily blue with red for the secondary bone influence
      color.xyz = vec3(boneWeight1, boneWeight0, 0);
    }
  }
}

#GLSL_FS
#version 440 core

// Trivial fragment shader that just returns the interpolated vertex color

precision mediump float;

layout(location = 0) in vec4 color; 

layout(location = 0) out vec4 fragColor;

void main()
{
  fragColor = color;
}
