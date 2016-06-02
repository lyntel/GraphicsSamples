//----------------------------------------------------------------------------------
// File:        es2-aurora\OptimizationApp/SceneFBOs.h
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

#ifndef SCENE_FBOS_H
#define SCENE_FBOS_H

#include <NvSimpleTypes.h>


#include "NvGLUtils/NvSimpleFBO.h"
#include "NV/NvPlatformGL.h"
#include "AppExtensions.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
#endif

// HACK - these are NOT the same value...
#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT GL_HALF_FLOAT_OES
#endif

class SceneFBOs
{
public:
    struct Params
    {
        Params()
        : particleDownsample(2.0f)
        , sceneDownsample(2.0f)
        {
        }

        float particleDownsample;
        float sceneDownsample;
    };

    SceneFBOs()
    : m_particleFbo(NULL)
    , m_sceneFbo(NULL)
    , m_backBufferFbo(NULL)
    , m_windowResX(0)
    , m_windowResY(0)
    {
    }

    ~SceneFBOs()
    {
        SAFE_DELETE(m_particleFbo);
        SAFE_DELETE(m_sceneFbo);
        SAFE_DELETE(m_backBufferFbo);
    }

    void setWindowSize(int32_t w, int32_t h)
    {
        m_windowResX = w;
        m_windowResY = h;

        updateParticleBuffer();
        updateSceneBuffer();

        SAFE_DELETE(m_backBufferFbo);
        m_backBufferFbo = new NvWritableFB(w,h);
    }

    void updateBuffers() {
        updateParticleBuffer();
        updateSceneBuffer();
    }

    void updateParticleBuffer()
    {
        NvSimpleFBO::Desc desc;
        desc.width  = (int32_t) ((float) m_windowResX 
            / (m_params.particleDownsample * m_params.sceneDownsample) + 0.5f);
        desc.height = (int32_t) ((float) m_windowResY 
            / (m_params.particleDownsample * m_params.sceneDownsample) + 0.5f);

        if (!m_particleFbo || (m_particleFbo->width != desc.width)  
            || (m_particleFbo->height != desc.height)) {
            desc.color.format = GL_RGBA;
            desc.color.type = GL_UNSIGNED_BYTE;
            desc.color.filter = GL_NEAREST;

            desc.depth.format = GL_DEPTH_COMPONENT;
            desc.depth.type = GL_UNSIGNED_INT;
            desc.depth.filter = GL_NEAREST;

            SAFE_DELETE(m_particleFbo);
            m_particleFbo = new NvSimpleFBO(desc);
        }
    }

    void updateSceneBuffer()
    {
        NvSimpleFBO::Desc desc;

        desc.width  = (int32_t) ((float) m_windowResX / m_params.sceneDownsample + 0.5f);
        desc.height  = (int32_t) ((float) m_windowResY / m_params.sceneDownsample + 0.5f);

        if (!m_sceneFbo || (m_sceneFbo->width != desc.width)  
            || (m_sceneFbo->height != desc.height)) {
            desc.color.format = GL_RGBA;
            desc.color.type = GL_UNSIGNED_BYTE;
            desc.color.filter = GL_NEAREST;

            desc.depth.format = GL_DEPTH_COMPONENT;
            desc.depth.type = GL_UNSIGNED_INT;
            desc.depth.filter = GL_NEAREST;

            SAFE_DELETE(m_sceneFbo);
            m_sceneFbo = new NvSimpleFBO(desc);
        }
    }

    Params m_params;
    int32_t m_windowResX;
    int32_t m_windowResY;

    NvSimpleFBO *m_particleFbo;
    NvSimpleFBO *m_sceneFbo;
    NvWritableFB *m_backBufferFbo;
};

#endif // SCENE_FBOS_H

