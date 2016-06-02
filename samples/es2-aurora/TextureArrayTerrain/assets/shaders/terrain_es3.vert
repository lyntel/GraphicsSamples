//----------------------------------------------------------------------------------
// File:        es2-aurora\TextureArrayTerrain\assets\shaders/terrain_es3.vert
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
#version 300 es
uniform mat4 g_modelViewMatrix;
uniform mat4 g_projectionMatrix;

in vec2  g_vPosition;            // Static attrs: only stores XZ here, Y below.
in vec4  g_vNormalAndHeight;        // Dynamic attrs: interleaved is most efficient.

out vec4 texCoord;
out vec3 normal;

void main()
{
    normal = g_vNormalAndHeight.xyz;
    float height = g_vNormalAndHeight.w;
    
    vec4 vPosEyeSpace = g_modelViewMatrix * vec4(g_vPosition.x, height, g_vPosition.y, 1.0);
    gl_Position = g_projectionMatrix * vPosEyeSpace;
    
    float u = g_vPosition.x / 8.0;
    float v = g_vPosition.y / 8.0;
    float slice = height / 1.5;
    float cliff = smoothstep(0.1, 0.8, 1.0-g_vNormalAndHeight.y);
    
    // Snow starts to fade into the slice below it.
    float snowSlice = 2.0;
    if (slice > snowSlice)
    {
        float snowAmount = slice - snowSlice;
        float snowAmountClamped = clamp(snowAmount, 0.0, 1.0);
        
        // A faster transition into snow, less fading.  Smaller values give faster transition; 1.0 gives the usual fade.
        float snowTransitionRate = 0.333;
        if (slice < snowSlice + 1.0)
            slice = snowSlice + (1.0 - snowTransitionRate) + snowTransitionRate * snowAmountClamped; 
        
        // Snow covers cliffs, eventually, with increasing height.
        if (snowAmount > 2.0)
            cliff = clamp(cliff - 0.2 * (snowAmount-2.0), 0.0, 1.0);
    }
    
    texCoord = vec4(u,v, slice, cliff);
}