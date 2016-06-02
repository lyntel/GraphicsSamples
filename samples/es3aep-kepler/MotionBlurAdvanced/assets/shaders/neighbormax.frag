//----------------------------------------------------------------------------------
// File:        es3aep-kepler\MotionBlurAdvanced\assets\shaders/neighbormax.frag
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

precision mediump float;

// OUTPUT
out vec4 o_vFragColor;

// UNIFORMS
uniform sampler2D u_tInputTex; // In BaseFullscreenShader

// CONSTANTS
const vec2 VHALF =  vec2(0.5, 0.5);
const vec2 VONE  =  vec2(1.0, 1.0);
const vec2 VTWO  =  vec2(2.0, 2.0);

const ivec2 IVZERO = ivec2(0, 0);
const ivec2 IVONE  = ivec2(1, 1);

const vec4 GRAY = vec4(0.5, 0.5, 0.5, 1.0);

// Bias/scale helper functions
vec2 readBiasScale(vec2 v)
{
    return v * VTWO - VONE;
}

vec2 writeBiasScale(vec2 v)
{
    return (v + VONE) * VHALF;
}

float compareWithNeighbor(ivec2 tileCorner, int s, int t, float maxMagnitudeSquared)
{
    ivec2 ivTexSizeMinusOne = textureSize(u_tInputTex, 0) - IVONE;
    ivec2 ivOffset = ivec2(s, t);
    vec2 vVelocity = readBiasScale(texelFetch(u_tInputTex,
        clamp(tileCorner + ivOffset, IVZERO, ivTexSizeMinusOne), 0).rg);
        float fMagnitudeSquared = dot(vVelocity, vVelocity);

    if(maxMagnitudeSquared < fMagnitudeSquared)
    {
        float fDisplacement = abs(float(ivOffset.x)) + abs(float(ivOffset.y));
        vec2 vOrientation = sign(vec2(ivOffset) * vVelocity);
        float fDistance = float(vOrientation.x + vOrientation.y);

        if(abs(fDistance) == fDisplacement)
        {
            o_vFragColor.rg = writeBiasScale(vVelocity);
            maxMagnitudeSquared = fMagnitudeSquared;
        }
    }
    return maxMagnitudeSquared;
}

void main()
{
    o_vFragColor = GRAY;

    ivec2 ivTileCorner = ivec2(gl_FragCoord.xy);
    float fMaxMagnitudeSquared = 0.0;

    fMaxMagnitudeSquared =
        compareWithNeighbor(ivTileCorner, -1, -1, fMaxMagnitudeSquared);
    fMaxMagnitudeSquared =
        compareWithNeighbor(ivTileCorner,  0, -1, fMaxMagnitudeSquared);
    fMaxMagnitudeSquared =
        compareWithNeighbor(ivTileCorner,  1, -1, fMaxMagnitudeSquared);
    fMaxMagnitudeSquared =
        compareWithNeighbor(ivTileCorner, -1,  0, fMaxMagnitudeSquared);
    fMaxMagnitudeSquared =
        compareWithNeighbor(ivTileCorner,  0,  0, fMaxMagnitudeSquared);
    fMaxMagnitudeSquared =
        compareWithNeighbor(ivTileCorner,  1,  0, fMaxMagnitudeSquared);
    fMaxMagnitudeSquared =
        compareWithNeighbor(ivTileCorner, -1,  1, fMaxMagnitudeSquared);
    fMaxMagnitudeSquared =
        compareWithNeighbor(ivTileCorner,  0,  1, fMaxMagnitudeSquared);
    fMaxMagnitudeSquared =
        compareWithNeighbor(ivTileCorner,  1,  1, fMaxMagnitudeSquared);
}