//----------------------------------------------------------------------------------
// File:        es2-aurora\ParticleUpsampling/SceneFBOs.h
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
#include "NV/NvMath.h"
#include "NvGLUtils/NvSimpleFBO.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
#endif

#ifndef GL_RED
#define GL_RED 0x1903
#endif

class SceneFBOs
{
public:
    struct Params
    {
        Params()
        : particleDownsample(4)
        , sceneDownsample(1)
        , lightBufferSize(64)
        {
        }

        uint32_t particleDownsample;
        uint32_t sceneDownsample;
        uint32_t lightBufferSize;
    };

    SceneFBOs()
    : m_lightFbo(NULL)
    , m_particleFbo(NULL)
    , m_sceneFbo(NULL)
    , m_sceneResX(0)
    , m_sceneResY(0)
    {
        createLightBuffer();
    }

    ~SceneFBOs()
    {
        SAFE_DELETE(m_lightFbo);
        SAFE_DELETE(m_particleFbo);
        SAFE_DELETE(m_sceneFbo);
    }

    void createScreenBuffers(int w, int h)
    {
        m_sceneResX  = w / m_params.sceneDownsample;
        m_sceneResY = h / m_params.sceneDownsample;
        createParticleBuffer();
        createSceneBuffer();
    }

    void createLightBuffer()
    {
        NvSimpleFBO::Desc desc;
        desc.width = m_params.lightBufferSize;
        desc.height = m_params.lightBufferSize;
        desc.color.format = GL_RED;
        desc.color.type = GL_UNSIGNED_BYTE;
        desc.color.filter = GL_LINEAR;
        desc.color.wrap = GL_CLAMP_TO_EDGE;

        SAFE_DELETE(m_lightFbo);
        m_lightFbo = new NvSimpleFBO(desc);
    }

    void createParticleBuffer()
    {
        NvSimpleFBO::Desc desc;
        desc.width = m_sceneResX   / m_params.particleDownsample;
        desc.height = m_sceneResY / m_params.particleDownsample;

        desc.color.format = GL_RGBA;
        desc.color.type = GL_UNSIGNED_BYTE;
        desc.color.filter = GL_NEAREST;

        desc.depth.format = GL_DEPTH_COMPONENT;
        desc.depth.type = GL_UNSIGNED_INT;
        desc.depth.filter = GL_NEAREST;

        SAFE_DELETE(m_particleFbo);
        m_particleFbo = new NvSimpleFBO(desc);
    }

    void createSceneBuffer()
    {
        NvSimpleFBO::Desc desc;
        desc.width = m_sceneResX;
        desc.height = m_sceneResY;

        desc.color.format = GL_RGBA;
        desc.color.type = GL_UNSIGNED_BYTE;
        desc.color.filter = GL_NEAREST;

        desc.depth.format = GL_DEPTH_COMPONENT;
        desc.depth.type = GL_UNSIGNED_SHORT;
        desc.depth.filter = GL_NEAREST;

        SAFE_DELETE(m_sceneFbo);
        m_sceneFbo = new NvSimpleFBO(desc);
    }

    Params m_params;
    int32_t m_sceneResX;
    int32_t m_sceneResY;

    NvSimpleFBO *m_lightFbo;
    NvSimpleFBO *m_particleFbo;
    NvSimpleFBO *m_sceneFbo;
};

#endif // SCENE_FBOS_H

