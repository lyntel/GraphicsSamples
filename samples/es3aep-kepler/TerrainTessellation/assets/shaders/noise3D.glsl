//----------------------------------------------------------------------------------
// File:        es3aep-kepler\TerrainTessellation\assets\shaders/noise3D.glsl
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
// 3D noise

uniform mediump sampler3D randTex3D;

// smooth interpolation curve
vec3 fade(vec3 t)
{
    //return t * t * (3 - 2 * t); // old curve (quadratic)
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); // new curve (quintic)
}

// returns value in [-1, 1]
float noise(vec3 p)
{
    return texture(randTex3D, p * invNoise3DSize).x*2.0-1.0;
}

vec4 noise4f(vec3 p)
{
    return texture(randTex3D, p * invNoise3DSize)*2.0-1.0;
}

// fractal sum
float fBm(vec3 p, int octaves, float lacunarity, float gain)
{
    float amp = 0.5;
    float sum = 0.0;    
    for(int i=0; i<octaves; i++) {
        sum += noise(p)*amp;
        //p = rotateMat * p;
        p *= lacunarity;
        amp *= gain;
    }
    return sum;
}

vec4 fBm4f(vec3 p, int octaves, float lacunarity, float gain)
{
    float amp = 0.5;
    vec4 sum = vec4(0.0);    
    for(int i=0; i<octaves; i++) {
        sum += noise4f(p)*amp;
        p *= lacunarity;
        amp *= gain;
    }
    return sum;
}

float turbulence(vec3 p, int octaves, float lacunarity, float gain)
{
    float amp = 0.5;
    float sum = 0.0;
    for(int i=0; i<octaves; i++) {
        sum += abs(noise(p))*amp;
        //p = rotateMat * p;
        p *= lacunarity;
        amp *= gain;
    }
    return sum;
}

vec4 turbulence4f(vec3 p, int octaves, float lacunarity, float gain)
{
    float amp = 0.5;
    vec4 sum = vec4(0.0);
    for(int i=0; i<octaves; i++) {
        sum += abs(noise4f(p))*amp;
        p *= lacunarity;
        amp *= gain;
    }
    return sum;
}
