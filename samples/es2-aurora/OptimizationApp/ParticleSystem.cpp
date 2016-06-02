//----------------------------------------------------------------------------------
// File:        es2-aurora\OptimizationApp/ParticleSystem.cpp
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
#include "IceRevisitedRadix.h"
#include "Perlin/ImprovedNoise.h"
#include <algorithm>

inline float frand()
{
    return rand() / (float) RAND_MAX;
}

inline float sfrand()
{
    return (float)(frand()*2.01-1.0);
}

inline vec3f randVec()
{
    return vec3f(sfrand(), sfrand(), sfrand());
}

float randInRange(float a, float b)
{ 
    return ((b - a) * ((float)rand() / RAND_MAX)) + a; 
}

static vec3f truncate(const vec4f& v)
{
    vec3f result(v.x, v.y, v.z);
    return result;
}

class ParticleInitializer
{
public:
    ParticleInitializer();
    virtual ~ParticleInitializer();

    int32_t getNumActive() const;

    // initialize particles in regular grid
    void initGrid(int32_t N);

    // add some procedural noise
    void addNoise(float freq, float scale);

    void setTime(float frameElapsed);
    void simulate();
    void depthSortEfficient(const vec3f& halfVector);

    vec4f *getPositions()                { return m_pos; }
    GLushort* getSortedIndices()        { return m_sortedIndices16; }

private:
    const float m_width;

    vec4f *m_pos;
    float *m_zs;
    GLushort *m_sortedIndices16;
    int32_t m_count;
    RadixSort m_sorter;

    int32_t m_numActive;
    ImprovedNoise m_noise;
    float m_elapsedTime;
};

ParticleInitializer::ParticleInitializer(): 
    m_pos(NULL), 
    m_zs(NULL),
    m_sortedIndices16(NULL),
    m_numActive(0), 
    m_width(480), 
    m_elapsedTime(0.0f) 
{
    const int32_t N = GRID_RESOLUTION;
    m_count = N * N;

    m_pos = new vec4f [m_count];
    m_zs = new float [m_count];
    m_sortedIndices16 = new GLushort [m_count];

    initGrid(GRID_RESOLUTION);
    addNoise(0.01, 70.0);
}

ParticleInitializer::~ParticleInitializer()
{
    delete [] m_pos;
    delete [] m_zs;
    delete [] m_sortedIndices16;
}

int32_t ParticleInitializer::getNumActive() const
{
    const int32_t result = m_numActive;
    return result;
}

ParticleSystem::ParticleSystem()
: m_pInit(NULL)
{
    // Although there is threading code for the init, it doesn't quite work because the VBO code in 
    // ParticleRenderer grabs a copy of the positions *once* before they are initialized.  Disabled for now.
    m_pInit = new ParticleInitializer();

}

ParticleSystem::~ParticleSystem()
{
    delete m_pInit;
}

float saturate(float f)
{
    return std::min(1.0f, std::max(0.0f, f));
}

// initialize particles in regular grid.  Single threaded.
void ParticleInitializer::initGrid(int32_t N)
{
    int32_t i = 0;
    for (int32_t z=0; z < N; z++)
    {
        for (int32_t x=0; x < N; x++)
        {
            vec3f p = vec3f(float(x), 0, float(z)) / vec3f(float(N), float(N), float(N));
            p = (p * 2.0f - 1.0f) * m_width;
            p.y = -1.0f;        // -2

            const vec3f coords(p.x, p.y, p.z);
            const float noise = 0.7f + fabs(m_noise.fBm(coords * 0.007)) * 2;
            m_pos[i] = vec4f(p.x, p.y, p.z, noise);
            i++;
        }
    }

    m_numActive = i;
    NV_ASSERT(m_numActive == N * N);
}

int32_t ParticleSystem::getNumActive() const
{
    return m_pInit->getNumActive();
}

// Single threaded.
void ParticleInitializer::addNoise(float freq, float scale)
{
    for (int32_t i = 0; i < m_numActive; i++)
    {
        const vec3f coords(truncate(m_pos[i]));
        const vec3f noise = m_noise.fBm3f(coords * freq) * scale;
        m_pos[i].x += noise.x;
        m_pos[i].y += noise.y;
        m_pos[i].z += noise.z;
    }
}

void ParticleInitializer::setTime(float frameElapsed)
{
    //m_timeCondition.Acquire();
    m_elapsedTime += frameElapsed;
    //m_timeCondition.Signal();
    //m_timeCondition.Release();
}

void ParticleSystem::simulate(float frameElapsed, const vec3f& halfVector, const vec4f& eyePos)
{
    m_pInit->setTime(frameElapsed);
    m_pInit->simulate();

    m_pInit->depthSortEfficient(halfVector);
}

static void wrapPos(float width, vec4f& pos)
{
    if (pos.x < -width)
        pos.x += 2*width;
    if (pos.x > width)
        pos.x -= 2*width;

    if (pos.z < -width)
        pos.z += 2*width;
    if (pos.z > width)
        pos.z -= 2*width;
}

void ParticleInitializer::simulate()
{
    //m_timeCondition.Acquire();
    //m_timeCondition.Wait();
    const float dt = m_elapsedTime;
    //m_timeCondition.Release();

    const vec4f wind(4.7f, 0.0f, 3.1f, 0.0f);
    for (int32_t i = 0; i < getNumActive(); i++)
    {
        m_pos[i] += dt * wind;
        wrapPos(m_width, m_pos[i]);
    }

    //m_timeCondition.Acquire();
    m_elapsedTime -= dt;
    //m_timeCondition.Release();
}

void ParticleInitializer::depthSortEfficient(const vec3f& halfVector)
{
    // calculate eye-space z
    for (int32_t i = 0; i < getNumActive(); ++i)
    {
        float z = -dot(halfVector, truncate(m_pos[i]));  // project onto vector
        m_zs[i] = z;
    }

    // sort
    m_sorter.Sort(m_zs, getNumActive());

    const GLuint *sortedIndices32 = m_sorter.GetIndices();
    for (int32_t i = 0; i < getNumActive(); ++i)
    {
        m_sortedIndices16[i] = (GLushort) sortedIndices32[i];
    }
}

static float distanceSqr(vec3f p1, vec3f p2)
{
    vec3f vec = p1 - p2;
    float dist = vec.x*vec.x + vec.y*vec.y + vec.z*vec.z;
    return dist;
}

class ParticleComparison2
{
public:
    ParticleComparison2(const vec4f* pPos, const vec3f& e): m_positions(pPos), m_eye(e) {}
    ParticleComparison2(const ParticleComparison2& pc): m_positions(pc.m_positions), m_eye(pc.m_eye) {}
    ParticleComparison2& operator=(const ParticleComparison2& pc) { m_positions = pc.m_positions; m_eye = pc.m_eye; return *this; }

    bool operator()(GLushort i1, GLushort i2)    
    { 
        // Is this so horribly naive that it's completely unrealistic?
        return distanceSqr(m_eye, truncate(m_positions[i1])) > distanceSqr(m_eye, truncate(m_positions[i2]));
    }
private:
    const vec4f* m_positions;
    vec3f m_eye;
};

vec4f* ParticleSystem::getPositions()
{
    return m_pInit->getPositions();
}

GLushort* ParticleSystem::getSortedIndices()
{
    return m_pInit->getSortedIndices();
}
