//----------------------------------------------------------------------------------
// File:        es2-aurora\TextureArrayTerrain\assets\shaders/terrain_es2.frag
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

#extension GL_EXT_gpu_shader4 : enable
#extension GL_EXT_texture_array : enable

precision mediump float;
uniform sampler2DArray g_terrainTex;
uniform float  g_interpOffset;

varying vec4 texCoord;
varying vec3 normal;

vec4 lerp(vec4 v1, vec4 v2, float i)
{
    return v1 + i * (v2-v1);
}

vec3 lerp(vec3 v1, vec3 v2, float i)
{
    return v1 + i * (v2-v1);
}

lowp vec3 cliffTex(lowp float greyBright)
{
    // Artistically, we want a dark, slate-grey texture for the cliff.  It is packed into the low-precision
    // alpha channel of DXT5.  So the texture is contrast stretched to the full [0,1] range and then
    // darkened here to the desired colour. 
    float cliffDarken = 0.6;
    float cliffGrey = cliffDarken * greyBright + 0.07;
    lowp vec3 cliff = vec3(cliffGrey, cliffGrey, cliffGrey);
    return cliff;
}

void main()
{
    // Interpolate between two slices.    
    lowp vec4 slice1 = texture2DArray(g_terrainTex, texCoord.xyz);
    lowp vec4 slice2 = texture2DArray(g_terrainTex, texCoord.xyz + vec3(0,0,1));
    mediump float interp = fract(texCoord.z - g_interpOffset);
    lowp vec3 baseColour = lerp(slice1.xyz, slice2.xyz, interp);
    
    // Assume cliff tex in alpha channel of every slice.
    lowp vec3 cliff = cliffTex(slice1.a); 

    // Three-way interpolation with a cliff texture.  texCoord.w is > 0 on areas of high slope.   
    lowp vec3 withCliff = lerp(baseColour, cliff, smoothstep(0.0, 1.0, texCoord.w));
    
    lowp float diff = 0.5 + 0.5 * dot(vec3(0.707, 0.707, 0.0), normal);        // Dumb, simple lighting.
    lowp vec3 colour = diff * withCliff;
    
    gl_FragColor = vec4(colour.x, colour.y, colour.z, 1);
    
    // For parameterization debugging:
    //gl_FragColor = vec4(0.2 * texCoord.x, 0.2 * texCoord.y, 0, 1);
}
