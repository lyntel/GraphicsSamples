//----------------------------------------------------------------------------------
// File:        es2-aurora\OptimizationApp/SceneRenderer.h
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

#ifndef SCENE_RENDERER_H
#define SCENE_RENDERER_H

#include <NvSimpleTypes.h>


#include "ParticleRenderer.h"
#include "Upsampler.h"
#include "SceneInfo.h"
#include "scene.h"
#include "Terrain.h"
#include "NvAppBase/NvCPUTimer.h"
#include "NvGLUtils/NvTimers.h"
#include "NvGLUtils/NvSimpleFBO.h"
#include "NV/NvStopWatch.h"

class OpaqueColorProgram;
class OpaqueDepthProgram;
class FillShadowMapProgram;

typedef enum
{
    GPU_TIMER_SCENE_DEPTH = 0,
    GPU_TIMER_PARTICLES,
    GPU_TIMER_SCENE_COLOR,
    GPU_TIMER_UPSAMPLE_PARTICLES,
    GPU_TIMER_UPSAMPLE_SCENE,
    GPU_TIMER_TOTAL,
    GPU_TIMER_COUNT
} GPUTimerId;

typedef enum
{
    CPU_TIMER_SCENE_DEPTH = 0,
    CPU_TIMER_PARTICLES,
    CPU_TIMER_SCENE_COLOR,
    CPU_TIMER_UPSAMPLE_PARTICLES,
    CPU_TIMER_UPSAMPLE_SCENE,
    CPU_TIMER_TOTAL,
    CPU_TIMER_COUNT
} CPUTimerId;

#define GPU_TIMER_SCOPE(TIMER_ID) NvGPUTimerScope gpuTimer(&m_GPUTimers[TIMER_ID])

#define CPU_TIMER_SCOPE(TIMER_ID) NvCPUTimerScope cpuTimer(&m_CPUTimers[TIMER_ID])

class SceneRenderer
{
public:
    struct Params
    {
        Params()
        : useDepthPrepass(false)
        , renderLowResolution(false)
        , backgroundColor(0.5f, 0.8f, 1.f)
        {
        }
        bool useDepthPrepass;
        bool renderLowResolution;
        vec3f backgroundColor;
    };

    SceneRenderer(bool isES2);
    ~SceneRenderer();

    void updateFrame(float frameElapsed);
    void initTimers();
    void renderFrame();
    bool stats(char* buffer, int32_t size);

    void reshapeWindow(int32_t w, int32_t h)
    {
        m_scene.setScreenSize(w, h);
        m_fbos->setWindowSize(m_scene.m_screenWidth, m_scene.m_screenHeight);
    }

    void setEyeViewMatrix(const nv::matrix4f& viewMatrix)
    {
        m_scene.m_eyeView = viewMatrix;
    }

    void setProjectionMatrix(const nv::matrix4f& m)
    {
        m_scene.m_eyeProj = m;
    }

    void setLightDirection(const nv::vec3f& d)
    {
        m_scene.setLightVector(d);
    }

    ParticleRenderer::Params *getParticleParams()
    {
        return &m_particles->getParams();
    }

    Upsampler::Params *getUpsamplingParams()
    {
        return &m_upsampler->getParams();
    }

    SceneFBOs::Params *getSceneFBOParams()
    {
        return &m_fbos->m_params;
    }

    SceneRenderer::Params *getSceneParams()
    {
        return &m_params;
    }

    int32_t getScreenWidth()
    {
        return m_scene.m_screenWidth;
    }

    int32_t getScreenHeight()
    {
        return m_scene.m_screenHeight;
    }

    SceneInfo m_scene;

//    void OSDAddStrings(OSD_type *osd);

private:
    // Note the values of RenderAlphaTest work with bit-tests ALL = ALPHA & SOLID.
    enum RenderAlphaTest { RENDER_NONE=0, RENDER_ALPHA=1, RENDER_SOLID=2, RENDER_ALL=3 };
    void drawScene(NvGLSLProgram&, const MatrixStorage&, RenderAlphaTest, bool a_renderDepth = false);
    void downsampleSceneDepth(const MatrixStorage&, NvSimpleFBO *srcFbo, NvWritableFB *dstFbo);
    void renderSceneDepth(const MatrixStorage&, NvWritableFB* depthFbo);
    void renderLowResSceneDepth(const MatrixStorage&);
    void renderFullResSceneColor(const MatrixStorage&);
    void setupMatrices(MatrixStorage&);
    void renderParticles(const MatrixStorage&);

    std::vector<MeshObj>            m_models;
    Terrain*                        m_pTerrain;
    std::map<std::string, NvModelGL*> m_modelStorage;
    std::map<std::string, GLuint  > m_texStorage;

    Params m_params;
    ParticleRenderer *m_particles;
    Upsampler *m_upsampler;

    NvGPUTimer m_GPUTimers[GPU_TIMER_COUNT];
    NvCPUTimer m_CPUTimers[CPU_TIMER_COUNT];

    const static int32_t STATS_FRAMES = 60;
    int32_t m_statsCountdown;

    int32_t m_screenWidth;
    int32_t m_screenHeight;
    SceneFBOs *m_fbos;

    OpaqueColorProgram *m_opaqueColorProg;
    OpaqueDepthProgram *m_opaqueSolidDepthProg;
    OpaqueDepthProgram *m_opaqueAlphaDepthProg;
};

#endif // SCENE_RENDERER_H
