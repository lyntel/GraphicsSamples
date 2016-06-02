//----------------------------------------------------------------------------------
// File:        es2-aurora\ParticleUpsampling\assets\shaders/upsampleCrossBilateral.frag
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

// Improve quality at "island" pixels (low-res pixel with no close-by high-res depth)
// Disabled to maximize performance
#define ENABLE_ISLAND_FIX 0

precision mediump float;

uniform vec2 g_depthConstants;
uniform vec2 g_lowResTextureSize;
uniform vec2 g_lowResTexelSize;
uniform float g_depthMult;
uniform float g_threshold;
uniform sampler2D g_fullResDepthTex;
uniform sampler2D g_lowResDepthTex;
uniform sampler2D g_lowResParticleColorTex;

varying vec2 texCoords;
varying vec2 uv00;
varying vec2 uv10;
varying vec2 uv01;
varying vec2 uv11;

float fetchLowResDepth(vec2 uv)
{
    return 1.0 / (texture2D(g_lowResDepthTex, uv).x * g_depthConstants.x + g_depthConstants.y);
}

float fetchFullResDepth(vec2 uv)
{
    return 1.0 / (texture2D(g_fullResDepthTex, uv).x * g_depthConstants.x + g_depthConstants.y);
}

void main()
{
    // the lowp specifiers below are used to reduce register pressure
    lowp vec4 c00 = texture2D(g_lowResParticleColorTex, uv00);
    lowp vec4 c10 = texture2D(g_lowResParticleColorTex, uv10);
    lowp vec4 c01 = texture2D(g_lowResParticleColorTex, uv01);
    lowp vec4 c11 = texture2D(g_lowResParticleColorTex, uv11);

    float z00 = fetchLowResDepth(uv00);
    float z10 = fetchLowResDepth(uv10);
    float z01 = fetchLowResDepth(uv01);
    float z11 = fetchLowResDepth(uv11);

    vec2 f = fract(uv00 * g_lowResTextureSize);
    vec2 g = vec2(1.0) - f;
    float w00 = g.x * g.y;
    float w10 = f.x * g.y;
    float w01 = g.x * f.y;
    float w11 = f.x * f.y;

    float zfull = fetchFullResDepth(texCoords);
    vec4 depthDelta = abs(vec4(z00,z10,z01,z11) - vec4(zfull));
    vec4 weights = vec4(w00,w10,w01,w11) / (depthDelta * g_depthMult + vec4(1.0));

    float weightSum = dot(weights, vec4(1.0));
    weights /= weightSum;
    
    lowp vec4 outputColor =    c00 * weights.x +
                            c10 * weights.y +
                            c01 * weights.z +
                            c11 * weights.w;

#if ENABLE_ISLAND_FIX
    if (weightSum < g_threshold)
    {
        lowp vec4 nearestDepthColor = c00;
        float dist = abs(z00 - zfull);
        float minDist = dist;
        dist = abs(z10 - zfull); if (dist < minDist) { minDist = dist; nearestDepthColor = c10; }
        dist = abs(z01 - zfull); if (dist < minDist) { minDist = dist; nearestDepthColor = c01; }
        dist = abs(z11 - zfull); if (dist < minDist) { minDist = dist; nearestDepthColor = c11; }
        outputColor = nearestDepthColor;
    }
#endif

    gl_FragColor = outputColor;
}
