//----------------------------------------------------------------------------------
// File:        vk10-kepler\ModelTestVk\assets\src_shaders/cube_map.glsl
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

layout(location = 0) in vec3 position;

layout(binding = 0) uniform Block {
    mat4 g_modelViewMatrix;
    mat4 g_projectionMatrix;
    mat4 g_invModelViewMatrix;
    mat4 g_invProjectionMatrix;
    vec3 g_modelLight;
};

out IO { vec3 v_vCubemapCoord; };

void main() {
  gl_Position = vec4(position.xy, 0.9999, 1.0);
  vec4 vPos = g_invProjectionMatrix * vec4(position.xy, -1.0, 1.0);
  vPos /= vPos.w;
  v_vCubemapCoord = (g_invModelViewMatrix * vec4(vPos.xyz, 0)).xyz;
}

#GLSL_FS
#version 440 core

layout(binding = 0) uniform Block {
    mat4 g_modelViewMatrix;
    mat4 g_projectionMatrix;
    mat4 g_invModelViewMatrix;
    mat4 g_invProjectionMatrix;
    vec3 g_modelLight;
};

layout(binding = 2) uniform samplerCube tex;

in IO { vec3 v_vCubemapCoord; };

layout(location = 0) out vec4 colorOut;

void main() {
  colorOut = texture(tex, v_vCubemapCoord);
//  colorOut = vec4((v_vCubemapCoord.xyz + 1.0) * 0.5, 1.0);
}
