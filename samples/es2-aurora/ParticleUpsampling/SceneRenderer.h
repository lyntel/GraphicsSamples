//----------------------------------------------------------------------------------
// File:        es2-aurora\ParticleUpsampling/SceneRenderer.h
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

#define ENABLE_GPU_TIMERS 0
#define ENABLE_CPU_TIMERS 0

#include <NvSimpleTypes.h>
#include "NV/NvPlatformGL.h"
#include "ParticleRenderer.h"
#include "Upsampler.h"
#include "SceneInfo.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NvGLUtils/NvSimpleFBO.h"
#include "NvGLUtils/NvTimers.h"

class OpaqueColorProgram;
class OpaqueDepthProgram;
class NvModelGL;

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
    CPU_TIMER_SORT = 0,
    CPU_TIMER_UPDATE_EBO,
    CPU_TIMER_SCENE_DEPTH,
    CPU_TIMER_PARTICLES,
    CPU_TIMER_SCENE_COLOR,
    CPU_TIMER_UPSAMPLE_PARTICLES,
    CPU_TIMER_UPSAMPLE_SCENE,
    CPU_TIMER_TOTAL,
    CPU_TIMER_COUNT
} CPUTimerId;

#if ENABLE_GPU_TIMERS
#define GPU_TIMER_SCOPE(TIMER_ID) NvGPUTimerScope gpuTimer(&m_GPUTimers[TIMER_ID])
#else
#define GPU_TIMER_SCOPE(TIMER_ID) {}
#endif

#if ENABLE_CPU_TIMERS
#define CPU_TIMER_SCOPE(TIMER_ID) NvCPUTimerScope cpuTimer(&m_CPUTimers[TIMER_ID])
#else
#define CPU_TIMER_SCOPE(TIMER_ID) {}
#endif


class SceneRenderer
{
public:
    struct Params
    {
        Params()
        : drawModel(true)
        , useDepthPrepass(false)
        , backgroundColor(0.5f, 0.8f, 1.0f)
        {
        }
        bool drawModel;
        bool useDepthPrepass;
        nv::vec3f backgroundColor;
    };

    SceneRenderer(bool isGL);
    ~SceneRenderer();

    void initTimers();
    void loadModelFromData(char *fileData);
    void loadModel();

    void drawScene(GLint positionAttrib, GLint normalAttrib = -1);
    void drawModel(GLint positionAttrib, GLint normalAttrib = -1);
    void drawFloor(GLint positionAttrib, GLint normalAttrib = -1);

    void downsampleSceneDepth(NvSimpleFBO *srcFbo, NvSimpleFBO *dstFbo);
    void renderSceneDepth(NvSimpleFBO* depthFbo);
    void renderLowResSceneDepth();
    void renderFullResSceneColor();
    void renderFrame();

    void reshapeWindow(int32_t w, int32_t h)
    {
        m_scene.setScreenSize(w, h);
        createScreenBuffers();
    }

    void createScreenBuffers()
    {
        m_fbos->createScreenBuffers(m_scene.m_screenWidth, m_scene.m_screenHeight);
    }

    void createLightBuffer()
    {
        m_fbos->createLightBuffer();
    }

    void setEyeViewMatrix(nv::matrix4f viewMatrix)
    {
        m_scene.m_eyeView = viewMatrix;
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

    void stats();

protected:
    Params m_params;
    NvModelGL *m_model;
    ParticleRenderer *m_particles;
    Upsampler *m_upsampler;
    SceneInfo m_scene;

#if ENABLE_GPU_TIMERS
    NvGPUTimer m_GPUTimers[GPU_TIMER_COUNT];
#endif
#if ENABLE_CPU_TIMERS
    NvCPUTimer m_CPUTimers[CPU_TIMER_COUNT];
#endif

    int32_t m_screenWidth;
    int32_t m_screenHeight;
    SceneFBOs *m_fbos;

    OpaqueColorProgram *m_opaqueColorProg;
    OpaqueDepthProgram *m_opaqueDepthProg;
};

#endif // SCENE_RENDERER_H
