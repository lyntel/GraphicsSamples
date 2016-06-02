//----------------------------------------------------------------------------------
// File:        es3aep-kepler\FeedbackParticlesApp/FeedbackParticlesScene.h
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
#ifndef _FEEDBACK_PARTICLES_SCENE_H
#define _FEEDBACK_PARTICLES_SCENE_H

#include <NvSimpleTypes.h>
#include <NV/NvPlatformGL.h>
#include <NV/NvMath.h>
#include "ParticleSystem.h"


//-----------------------------------------------------------------------------
//
class FeedbackParticlesScene
{
    friend class FeedbackParticlesApp;
public:
    bool            init();
    void            release();
    void            update(float deltaTime);
    void            draw();

    void            setView(const nv::matrix4f& mMV) { m_modelView = mMV; };
    void            setProjection(const nv::matrix4f& mProj) { m_projection = mProj; };
    ParticleSystem::Parameters& 
                    getParticleParamsRef() { return m_particleSystem.getParametersRef(); };
    uint32_t        getParticlesCount() { return m_particleSystem.getParticlesCount(); };

protected:
    bool            m_isSimulationStopped;
    bool            m_reset;

private:
    ParticleSystem  m_particleSystem;
    
    nv::matrix4f    m_modelView;
    nv::matrix4f    m_projection;
};

#endif
