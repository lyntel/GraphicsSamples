//----------------------------------------------------------------------------------
// File:        es2-aurora\OptimizationApp\assets\shaders/base_es2.vert
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
#version 100
uniform mat4 g_ModelViewMatrix;
uniform mat4 g_ModelViewProjectionMatrix;
uniform mat4 g_LightModelViewMatrix;

uniform vec3  g_lightDirection;

attribute vec4 g_Position;
attribute vec3 g_Normal;
attribute vec2 g_TexCoord;

varying vec3  normal;
varying float lightDepth;
varying vec2  textureCoord;

void main()
{
  vec3 posLighSpace  = (g_LightModelViewMatrix  * g_Position).xyz;

  gl_Position    = g_ModelViewProjectionMatrix  * g_Position;
  normal         = mat3(g_ModelViewMatrix)      * g_Normal;
  lightDepth     = posLighSpace.z;
  textureCoord   = vec2(g_TexCoord.x, 1.0-g_TexCoord.y);
}
