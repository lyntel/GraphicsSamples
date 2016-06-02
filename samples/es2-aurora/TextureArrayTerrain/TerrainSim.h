//----------------------------------------------------------------------------------
// File:        es2-aurora\TextureArrayTerrain/TerrainSim.h
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

#ifndef _TERRAIN_SIM_
#define _TERRAIN_SIM_

#include <NvSimpleTypes.h>


#include <iostream>
#include "Array2D.h"
#include "NV/NvMath.h"

// Fill a height field with terrain-like heights using fBm and ridge noise, etc.  This is not really
// the main point of the sample - which is terrain texturing.  But we need some plausible data on which
// to place the texture.  Hence we don't go to great lengths to create something truly realistic.
class TerrainSim
{
public:
    TerrainSim(int32_t w, int32_t h, const nv::vec2f& trans);
    ~TerrainSim();

    // Flatten all heights to 0.
    void reset();

    struct Params
    {
        int32_t octaves;
        float heightScale, heightOffset;
        float ridgeOffset, uvOffset;

        Params(): octaves(8), heightScale(1), heightOffset(0), ridgeOffset(1), uvOffset(0)    {}
    };

    void initParams(const Params&);
    void setParams(const Params&);
    bool dirtyParams() { return m_dirty; }

    // Populate with noise from fBm etc.  Calculate normals also.
    void simulate();

    int32_t getWidth() { return m_width; }
    int32_t getHeight() { return m_height; }
    float *getHeightField() { return m_u.data; }
    float *getNormals() { return m_normals; }

    // Total numbers of floats (or whatever) in the arrays.
    size_t totalHeightFieldElements() const    { return m_width * m_height; }
    size_t totalNormalElements() const        { return m_width * m_height * 3; }

private:
    TerrainSim() {}
    float getUnbounded(int32_t x, int32_t y) const;
    float computeTerrain(int32_t x, int32_t y) const;
    void calcNormals();

    int32_t m_width, m_height;
    float m_recipW, m_recipH;
    Array2D<float> m_u;
    float *m_normals;

    nv::vec2f m_translation;

    Params m_params;
    bool m_dirty;
};

inline void TerrainSim::setParams(const Params& p)
{
    if( ( p.heightOffset != m_params.heightOffset ) ||
        ( p.heightScale != m_params.heightScale ) ||
        ( p.octaves != m_params.octaves ) ||
        ( p.ridgeOffset != m_params.ridgeOffset ) ||
        ( p.uvOffset != m_params.uvOffset ) )
    {
        m_dirty = true;
        m_params = p;
    }
}

inline void TerrainSim::initParams(const Params& p)
{
    m_dirty = true;
    m_params = p;
}

#endif
