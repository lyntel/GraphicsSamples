//----------------------------------------------------------------------------------
// File:        es3aep-kepler\MotionBlurAdvanced\assets\shaders/skyboxvelocity.frag
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

precision highp float;

// INPUT
in vec4 v_vCurrentCubemapCoord;
in vec4 v_vPreviousCubemapCoord;

// OUTPUT
out vec4 o_vFragColor;

// UNIFORMS

// This value represents the (1/2 * exposure * framerate) part of the qX
// equation.
// NOTE ABOUT EXPOSURE: It represents the fraction of time that the exposure
//                      is open in the camera during a frame render. Larger
//                      values create more motion blur, smaller values create
//                      less blur. Arbitrarily defined by paper authors' in
//                      their source code as 75% and controlled by a slider
//                      in their demo.
uniform float u_fHalfExposureXFramerate; // In BaseVelocityShader
uniform float u_fK;                      // In BaseVelocityShader

// Constants
#define EPSILON 0.001
const vec2 VHALF =  vec2(0.5, 0.5);
const vec2 VONE  =  vec2(1.0, 1.0);
const vec2 VTWO  =  vec2(2.0, 2.0);

// Bias/scale helper functions
vec2 readBiasScale(vec2 v)
{
    return v * VTWO - VONE;
}

vec2 writeBiasScale(vec2 v)
{
    return (v + VONE) * VHALF;
}

void main()
{
    vec4 vPosCurr = normalize(v_vCurrentCubemapCoord)  * 0.999;
    vec4 vPosPrev = normalize(v_vPreviousCubemapCoord) * 0.999;

    vec2 vQX = ((vPosCurr.xy/vPosCurr.w) - (vPosPrev.xy/vPosPrev.w)) *
                u_fHalfExposureXFramerate;
    float fLenQX = length(vQX);

    float fWeight = max(0.5, min(fLenQX, u_fK));
    fWeight /= ( fLenQX + EPSILON );

    vQX *= fWeight;

    o_vFragColor = vec4(writeBiasScale(vQX), 0.5, 1.0); // <- 0.5 to keep gray at 0
}
