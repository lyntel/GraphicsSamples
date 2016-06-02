//----------------------------------------------------------------------------------
// File:        es2-aurora\TextureArrayTerrain/TerrainSim.cpp
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

#include "TerrainSim.h"
#include "RidgedMultiFractal.h"
#include "NV/NvLogs.h"

#undef NDEBUG

static ImprovedNoise g_noiseGen;

TerrainSim::TerrainSim(int32_t w, int32_t h, const nv::vec2f& trans) :
    m_width(w),
    m_height(h),
    m_recipW(1.0f / (float) (m_width-1)),
    m_recipH(1.0f / (float) (m_height-1)),
    m_translation(trans),
    m_dirty(false)
{
    m_u.init(m_width,m_height);
    m_normals = new float[m_width*m_height*3];
    reset();
}

TerrainSim::~TerrainSim()
{
    delete [] m_normals;
}

void TerrainSim::reset()
{
    int32_t size = m_width * m_height;
    for(int32_t i=0; i<size; i++)
        m_u.data[i] = 0.0f;
}

float TerrainSim::computeTerrain(int32_t x, int32_t y) const
{
    nv::vec2f uv((float)x * m_recipW, (float)y * m_recipH);

    // Translation places this tile relative to all the others.  uvOffset is a user adjustable offset.
    uv -= (m_translation + nv::vec2f(m_params.uvOffset));

    const float h = (1+m_params.ridgeOffset) * hybridTerrain(g_noiseGen, uv, m_params.octaves, m_params.ridgeOffset);
    return m_params.heightScale * h + m_params.heightOffset;
}

void TerrainSim::simulate()
{
    // Too much spew for regular use.
    // LOGI("Simulate tile at (%f,%f)", m_translation.x, m_translation.y);

    // The dirty flag is set *asynchronously* in our setParams method, in response to slider touches.  We need 
    // to complete one complete iteration of the loop without the params being modified by the GUI.  There *might*
    // be a race condition on setting/reading m_dirty but it's not critical.  I can live with that.
    while (m_dirty)
    {
        m_dirty = false;

        // Note that we deliberately allow the parameters to be asynchronously set while executing these for loops.  
        // We maintain GUI interactivity with the compromise of visual discontinuities while the parameters are 
        // being altered (which is acceptable).  A final outer while loop execution with m_dirty == false will
        // fix the discontinuities after the params stop changing.
        for(int32_t j=0; j<m_height; j++)
            for(int32_t i=0; i<m_width; i++)
                m_u.get(i, j) = computeTerrain(i,j);

        // m_dirty may have been asynchronously set true by now.  If so, do another iteration round the while loop.
    }

    calcNormals();
}

float TerrainSim::getUnbounded(int32_t x, int32_t y) const
{
    // Gradient calculation requires data outside that stored in m_u.  As an alternative, we could
    // store a margin in m_u and make the triangle indexing more complex in TerrainSimRenderer.
    if (x >= 0 && x < m_width && y >= 0 && y < m_height)
        return m_u.get(x,y);
    else
        return computeTerrain(x,y);
}

void TerrainSim::calcNormals()
{
    float *ptr = NULL;
    for(int32_t j=0; j<m_height; j++)
    {
        ptr = m_normals + j*m_width*3;
        for(int32_t i=0; i<m_width; i++)
        {
            const float dx = getUnbounded(i+1, j) - getUnbounded(i-1, j);    //dy/dx
            const float dz = getUnbounded(i, j+1) - getUnbounded(i, j-1);    //dy/dz
            nv::vec3f n(2.0f * dx, 1, 2.0f * dz);                            // Why the 2x?
            n = normalize(n);

            *ptr++ = n.x;
            *ptr++ = n.y;
            *ptr++ = n.z;

            NV_ASSERT(ptr <= m_normals + totalNormalElements());        // Don't overrun.
        }
    }
    NV_ASSERT(ptr == m_normals + totalNormalElements());        // Fill entire buffer with no underrun.
}
