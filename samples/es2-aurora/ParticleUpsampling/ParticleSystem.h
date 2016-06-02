//----------------------------------------------------------------------------------
// File:        es2-aurora\ParticleUpsampling/ParticleSystem.h
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

#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <NvSimpleTypes.h>
#include "IceRevisitedRadix.h"
#include "NV/NvMath.h"
#include "NV/NvPlatformGL.h"
#include "Perlin/ImprovedNoise.h"

using namespace nv;

// The number of particles is GRID_RESOLUTION cubed.  Particles are arranged in a regular cube,
// and then jittered by a high degree of noise.  If you increase the number of particles, their
// size must be decreased to get a similar visual result and overdraw cost.
#define HIGH_QUALITY 0
#if HIGH_QUALITY
#define GRID_RESOLUTION 32
#define PARTICLE_SCALE 0.5f
#else
#define GRID_RESOLUTION 16
#define PARTICLE_SCALE 1.f
#endif

class ParticleSystem
{
public:
    ParticleSystem();
    ~ParticleSystem();

    // initialize particles in regular grid
    void initGrid(int32_t N);

    // add some procedural noise
    void addNoise(float freq, float scale);

    // sort the particles along the halfVector direction
    void depthSort(vec3f halfVector);
    
    vec3f *getPositions()
    {
        return m_pos;
    }
    
    GLushort* getSortedIndices()
    {
        return m_sortedIndices16;
    }

    int32_t getNumActive()
    {
        return m_numActive;
    }

private:
    vec3f *m_pos;
    float *m_zs;
    GLushort *m_sortedIndices16;

    int32_t m_count;
    int32_t m_numActive;

    ImprovedNoise m_noise;
    RadixSort m_sorter;
};

#endif // PARTICLE_SYSTEM_H
