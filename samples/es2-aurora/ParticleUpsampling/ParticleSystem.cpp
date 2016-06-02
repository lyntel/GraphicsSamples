//----------------------------------------------------------------------------------
// File:        es2-aurora\ParticleUpsampling/ParticleSystem.cpp
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

#include "ParticleSystem.h"

inline float frand()
{
    return rand() / (float) RAND_MAX;
}

inline float sfrand()
{
    return frand()*2.01f-1.0f;
}

inline nv::vec3f randVec()
{
    return nv::vec3f(sfrand(), sfrand(), sfrand());
}

ParticleSystem::ParticleSystem()
: m_numActive(0)
{
    const int32_t N = GRID_RESOLUTION;
    m_count = N * N * N;

    m_pos = new nv::vec3f [m_count];
    m_zs = new float [m_count];
    m_sortedIndices16 = new GLushort [m_count];

    initGrid(N);
    addNoise(1.9, 1.0);
}

ParticleSystem::~ParticleSystem()
{
    delete [] m_pos;
    delete [] m_zs;
    delete [] m_sortedIndices16;
}

// initialize particles in regular grid
void ParticleSystem::initGrid(int32_t N)
{
    const float r = 1.f;
    int32_t i = 0;
    for (int32_t z = 0; z < N; z++)
    {
        for (int32_t y=0; y < N; y++)
        {
            for (int32_t x=0; x < N; x++)
            {
                nv::vec3f p = nv::vec3f((float)x, (float)y, (float)z) / nv::vec3f((float)N, (float)N, (float)N);
                p = (p * 2.0f - 1.0f) * r;
                if (i < m_count)
                {
                    m_pos[i] = p;
                    i++;
                }
            }
        }
    }
    m_numActive = i;
}

void ParticleSystem::addNoise(float freq, float scale)
{
    for (int32_t i = 0; i < m_count; i++)
    {
        m_pos[i] += m_noise.fBm3f(m_pos[i] * freq) * scale;
    }
}

void ParticleSystem::depthSort(vec3f halfVector)
{
    // calculate eye-space z
    for (int32_t i = 0; i < m_numActive; ++i)
    {
        float z = -dot(halfVector, m_pos[i]);  // project onto vector
        m_zs[i] = z;
    }

    // sort
    m_sorter.Sort(m_zs, (udword)m_numActive);

    const GLuint *sortedIndices32 = (GLuint*)m_sorter.GetIndices();
    for (int32_t i = 0; i < m_numActive; ++i)
    {
        m_sortedIndices16[i] = (GLushort) sortedIndices32[i];
    }
}
