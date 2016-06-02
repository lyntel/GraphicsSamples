//----------------------------------------------------------------------------------
// File:        es3aep-kepler\TerrainTessellation\assets\shaders/noise.glsl
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
// 2D noise

uniform mediump sampler2D randTex;

// smooth interpolation curve
vec2 fade(vec2 t)
{
    //return t * t * (3 - 2 * t); // old curve (quadratic)
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); // new curve (quintic)
}

// derivative of fade function
vec2 dfade(vec2 t)
{
    return 30.0*t*t*(t*(t-2.0)+1.0); // new curve (quintic)
}

// 2D noise using 2D texture lookup
// note - artifacts may be visible at low frequencies due to hardware interpolation precision

// returns value in [-1, 1]
float noise(vec2 p)
{
    return texture(randTex, p * invNoiseSize).x*2.0-1.0;
}

// with smooth software interpolation

float smoothnoise(vec2 p)
{
    vec2 i = floor(p);
      vec2 f = p - float(i);
    f = fade(f);

    // use texture gather - returns 4 neighbouring texels in a single lookup
    // note weird ordering of returned components!
    vec4 n = textureGather(randTex, i * invNoiseSize)*2.0-1.0;
    return mix( mix( n.w, n.z, f.x),
                mix( n.x, n.y, f.x), f.y);
}

// 2D noise with derivatives
// returns derivative in xy, normal noise value in z
// http://www.iquilezles.org/www/articles/morenoise/morenoise.htm

vec3 dnoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 u = p - i;

    vec2 du = dfade(u);
    u = fade(u);

    // get neighbouring noise values
    vec4 n = textureGather(randTex, i * invNoiseSize)*2.0-1.0;

    // rename components for convenience
    float a = n.w;
    float b = n.z;
    float c = n.x;
    float d = n.y;

    float k0 = a;
    float k1 = b - a;
    float k2 = c - a;
    float k3 = a - b - c + d;

    vec3 r;
    // noise derivative
    r.xy = (vec2(k1, k2) + k3*u.yx) * du;
    // noise value
    r.z = k0 + k1*u.x + k2*u.y + k3*u.x*u.y;
    return r;
}

// rotate octaves to avoid axis-aligned artifacts
const mat2 rotateMat = mat2(1.6, -1.2, 1.2, 1.6);

// fractal sum
float fBm(vec2 p, int octaves, float lacunarity, float gain)
{
    float amp = 0.5;
    float sum = 0.0;    
    for(int i=0; i<octaves; i++) {
        sum += noise(p)*amp;
        p = rotateMat * p;
        //p *= lacunarity;
        amp *= gain;
    }
    return sum;
}

// fbm with gradients (iq style)
float fBmGrad(vec2 p, int octaves, float lacunarity, float gain)
{
    float amp = 0.5;
    vec2 d = vec2(0.0);
    float sum = 0.0;
    for(int i=0; i<octaves; i++) {
        vec3 n = dnoise(p);
        d += n.xy;
        sum += n.z*amp / (1.0 + dot(d, d));   // sum scaled by gradient
        amp *= gain;
        //p *= lacunarity;
        p = rotateMat * p;
    }
    return sum;
}

float turbulence(vec2 p, int octaves, float lacunarity, float gain)
{
    float sum = 0.0;
    float amp = 0.5;
    for(int i=0; i<octaves; i++) {
        sum += abs(noise(p))*amp;
        //p *= lacunarity;
        p = rotateMat * p;
        amp *= gain;
    }
    return sum;
}

// Ridged multifractal
// See "Texturing & Modeling, A Procedural Approach", Chapter 12
float ridge(float h, float offset)
{
    h = abs(h);     // create creases
    h = offset - h; // invert so creases are at top
    h = h * h;      // sharpen creases
    return h;
}

float ridgedMF(vec2 p, int octaves, float lacunarity, float gain, float offset)
{
    float sum = 0.0;
    float freq = 1.0, amp = 0.5;
    float prev = 1.0;
    for(int i=0; i<octaves; i++) {
        float n = ridge(smoothnoise(p*freq), offset);
        //sum += n*amp;
        sum += n*amp*prev;  // scale by previous octave
        prev = n;
        freq *= lacunarity;
        amp *= gain;
    }
    return sum;
}

// mixture of ridged and fbm noise
float hybridTerrain(vec2 x, int octaves)
{
    float h = ridgedMF(x, 2, 2.0, 0.5, 1.1);
    float f = fBm(x * 4.0, octaves - 2, 2.0, 0.5) * 0.5;
    return h + f*h;
}
