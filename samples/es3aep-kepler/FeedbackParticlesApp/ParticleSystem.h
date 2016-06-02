//----------------------------------------------------------------------------------
// File:        es3aep-kepler\FeedbackParticlesApp/ParticleSystem.h
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
#ifndef _PARTICLE_SYSTEM_H
#define _PARTICLE_SYSTEM_H

#include <NvSimpleTypes.h>
#include <NV/NvMath.h>
#include "Utils.h"


//------------------------------------------------------------------------------
//
class NvGLSLProgram;


//------------------------------------------------------------------------------
//
class ParticleSystem
{
    struct Particle
    {
        enum
        {
            EMITTER = 1,
            NORMAL,
        };

        uint8_t     type;
        uint8_t     color[3];
        float       age;
        nv::vec3f   position;
        nv::vec3f   velocity;
    };

    enum
    {
        RANDOM_TEX_UNIT,
        FBM_TEX_UNIT,
        PARTICLE_TEX_UNIT   = 0,

        RANDOM_TEX_SIZE     = 2048,
        FBM_TEX_SIZE        = 256,
        PARTICLE_TEX_SIZE   = 256,
        MAX_PARTICLES       = (1024*1024),

        FEEDBACK_QUEUE_LEN  = 2,
        EMITTERS_FEEDBACK   = FEEDBACK_QUEUE_LEN,
        MAX_EMITTERS        = 128,
    };

    typedef Ring<FEEDBACK_QUEUE_LEN> RingCycle;

public:
    struct Parameters
    {
        Parameters()
        {
            emittersCount     = 4;
            emitPeriod        = 5.f/1000.f; //5ms
            emitCount         = 40;
            particleLifetime  = 10;
            billboardSize     = 0.01f;
            velocityScale     = 1.f;
            freezeEmitters    = false;
            removeNoise       = false;
            freezeColliders   = false;
            useColors         = false;
        };

        uint32_t emittersCount;
        float    emitPeriod;
        uint32_t emitCount;
        float    particleLifetime;
        float    billboardSize;
        float    velocityScale;
        bool     freezeEmitters;
        bool     freezeColliders;
        bool     removeNoise;
        bool     useColors;
    };

    ParticleSystem();

    bool            init();
    void            release();
    void            update(float deltaTime);
    void            draw(const nv::matrix4f& mMV, const nv::matrix4f& mProj);

    void            stopSimulation(bool b=true) { m_isStopped = b; };
    Parameters&     getParametersRef() { return m_params; };
    uint32_t        getParticlesCount() { return m_particleCount; };

private:
    void            advanceFeedback();
    void            drawParticles(const nv::matrix4f& mMV, const nv::matrix4f& mProj);
    bool            initFeedbackProgram();
    bool            initBillboardProgram();
    void            initParticleBuffers();
    void            setDefaultValues();
    bool            initEmitterProgram();
    void            updateEmitters();
    void            emitParticles();
    void            processParticles();
    void            moveEmitters();
    void            seedEmitters(uint32_t num);

    NvGLSLProgram*  m_feedbackProgram;
    NvGLSLProgram*  m_billboardProgram;
    NvGLSLProgram*  m_emitterProgram;

    uint32_t        m_randomTexture;
    uint32_t        m_particleTexture;
    uint32_t        m_FBMTexture;

    uint32_t        m_emittersBuffer;
    uint32_t        m_particlesBuffers[FEEDBACK_QUEUE_LEN + 1];     //+one for emitters feedback
    uint32_t        m_transformFeedback[FEEDBACK_QUEUE_LEN + 1];
    uint32_t        m_countQuery[FEEDBACK_QUEUE_LEN];
    
    RingCycle       m_ring;

    bool            m_isStopped;

    float           m_time;
    float           m_deltaTime;
    float           m_collidersTime;

    Parameters      m_params;
    Particle        m_emitters[MAX_EMITTERS];
    uint32_t        m_emittersCount;
    uint32_t        m_particleCount;

    static NvVertexAttribute ms_attributes[];
};

#endif
