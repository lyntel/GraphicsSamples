//----------------------------------------------------------------------------------
// File:        es2-aurora\TextureArrayTerrain/RidgedMultiFractal.h
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

#ifndef _RIDGED_MULTI_FRACTAL_
#define _RIDGED_MULTI_FRACTAL_

#include <NvSimpleTypes.h>


#include "Perlin/ImprovedNoise.h"

// Ridged multifractal
// See Kenton Musgrave (2002). "Texturing and Modeling, Third Edition: A Procedural Approach." Morgan Kaufmann.
float ridge(float h, float offset)
{
    float result = fabs(h);
    result = offset - result;
    result = result * result;
    return result;
}

float ridgedMF(ImprovedNoise& gen, vec2f p, int32_t octaves, float lacunarity = 2.0, float gain = 0.5, float offset = 1.0)
{
    // Hmmm... these hardcoded constants make it look nice.  Put on tweakable sliders?
    vec3f p3(p.x, p.y, 0.0);
    float f = 10.0f * gen.fBm(p3, octaves, lacunarity, gain);
    return ridge(f, offset);
}

float saturate(float f)
{
    return std::min(1.0f, std::max(0.0f, f));
}

// mixture of ridged and fbm noise
float hybridTerrain(ImprovedNoise& gen, vec2f x, int32_t octaves, float ridgeOffset)
{
    const vec3f x3(x.x, x.y, 0.0);
    //const int32_t RIDGE_OCTAVES = g_ridgeOctaves;
    //const int32_t FBM_OCTAVES   = g_fBmOctaves;
    //const int32_t TWIST_OCTAVES = g_twistOctaves;
    const float LACUNARITY = 2, GAIN = 0.5;

    // Distort the ridge texture coords.  Otherwise, you see obvious texel edges.
    //vec2f xOffset = vec2f(gen.fBm(0.2*x3, TWIST_OCTAVES), gen.fBm(0.2*x3+0.2, TWIST_OCTAVES));
    //vec2f xTwisted = x + 0.1 * xOffset;

    //const float fBm_UVScale  = powf(LACUNARITY, RIDGE_OCTAVES);
    //const float fBm_AmpScale = powf(GAIN,       RIDGE_OCTAVES);
    //float f = 10.0f * gen.fBm(x3 * fBm_UVScale, FBM_OCTAVES, LACUNARITY, GAIN) * fBm_AmpScale;
    //float f = 2.0f * gen.fBm(x3, FBM_OCTAVES, LACUNARITY, GAIN);

    // Ridged is too ridgy.  So interpolate between ridge and fBm for the coarse octaves.
    float h = ridgedMF(gen, x, octaves, LACUNARITY, GAIN, ridgeOffset);
    return h;

    /*
    if (RIDGE_OCTAVES > 0)
        return g_ridgeContribution * (h - f) + f;
    else
        return f;
        */
}

#endif
