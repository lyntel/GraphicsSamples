//----------------------------------------------------------------------------------
// File:        es2-aurora\ParticleUpsampling/ParticleRenderer.h
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

#ifndef PARTICLE_RENDERER_H
#define PARTICLE_RENDERER_H

#include <NvSimpleTypes.h>
#include "NV/NvMath.h"
#include "NV/NvPlatformGL.h"
#include "SceneFBOs.h"
#include "ParticleSystem.h"
#include "SceneInfo.h"

class LightViewParticleProgram;
class CameraViewParticleProgram;

class ParticleRenderer
{
public:
    struct Params
    {
        Params()
        : renderShadows(true)
        , numSlices(16)
        , softness(0.4f)
        , particleScale(PARTICLE_SCALE)
        , pointRadius(0.2f)
        , shadowAlpha(0.04f)
        , spriteAlpha(0.2f)
        {
        }

        float getPointScale(const nv::matrix4f &projectionMatrix)
        {
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);

            float worldSpacePointRadius = pointRadius * particleScale;
            float projectionScaleY = projectionMatrix.element(1,1);
            float viewportHeight = float(viewport[3]);

            return worldSpacePointRadius * projectionScaleY * viewportHeight;
        }

        bool renderShadows;
        uint32_t numSlices;
        float softness;
        float particleScale;
        float pointRadius;
        float shadowAlpha;
        float spriteAlpha;
    };

    ParticleRenderer(bool isGL);
    ~ParticleRenderer();

    void createShaders();
    void deleteShaders();

    void drawPointsSorted(GLint positionAttrib, int32_t start, int32_t count);
    void drawSliceCameraView(SceneInfo& scene, int32_t i);
    void drawSliceLightView(SceneInfo& scene, int32_t i);

    void depthSort(SceneInfo& scene);
    void renderParticles(SceneInfo& scene);

    void deleteVBO();
    void createVBO();

    void deleteEBOs();
    void createEBOs();
    void updateEBO();

    int32_t getNumActive()
    {
        return m_particleSystem->getNumActive();
    }

    void swapBuffers()
    {
        m_frameId = (m_frameId + 1) % m_eboCount;
    }

    Params& getParams()
    {
        return m_params;
    }

private:
    Params m_params;
    ParticleSystem *m_particleSystem;
    int32_t m_batchSize;

    LightViewParticleProgram *m_lightViewParticleProg;
    CameraViewParticleProgram *m_cameraViewParticleProg;

    GLuint m_vbo;
    GLuint m_frameId;
    GLuint *m_eboArray;
    int32_t m_eboCount;
    bool m_isGL;
};

#endif // PARTICLE_RENDERER_H
