//----------------------------------------------------------------------------------
// File:        gl4-kepler\WeightedBlendedOIT/WeightedBlendedOIT.cpp
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
#include "WeightedBlendedOIT.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NvGLUtils/NvModelGL.h"
#include "NvModel/NvModel.h"
#include "NvGLUtils/NvShapesGL.h"
#include "NvAppBase/NvInputTransformer.h"
#include <sstream>
#include <iomanip>

#define MAX_DEPTH 1.0
#define MAX_PEELED_LAYERS 64
#define NUM_FRAMES_PER_GPU_TIME 30

const static float g_white[3] = {1.0,1.0,1.0};
const static float g_sky[3] = {0.1,0.3,0.7};
const static float g_black[3] = {0.0};

// The sample has a dropdown menu in the TweakBar that allows the user to
// select a specific stage or pass of the algorithm for visualization.
// These are the options in that menu (for more details, please see the
// sample documentation)
static NvTweakEnum<uint32_t> ALGORITHM_OPTIONS[] =
{
    {"Depth Peeling", WeightedBlendedOIT::DEPTH_PEELING_MODE},
    {"Weighted Blended", WeightedBlendedOIT::WEIGHTED_BLENDED_MODE}
};

static NvTweakEnum<uint32_t> MODEL_OPTIONS[] =
{
    {"Bunny", WeightedBlendedOIT::BUNNY_MODEL},
    {"Dragon", WeightedBlendedOIT::DRAGON_MODEL},
};

WeightedBlendedOIT::WeightedBlendedOIT() :
    NvSampleAppGL(platform, "Weighted Blended OIT"), 
    m_imageWidth(0),
    m_imageHeight(0),
    m_opacity(0.6f),
    m_mode(WEIGHTED_BLENDED_MODE),
    m_numGeoPasses(0),
    m_backgroundColor(g_sky),
    m_weightParameter(0.5f)
{
    m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -2.5f));

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

WeightedBlendedOIT::~WeightedBlendedOIT()
{
    LOGI("WeightedBlendedOIT: destroyed\n");
}

bool WeightedBlendedOIT::getRequestedWindowSize(int32_t& width, int32_t& height)
{
    width = 1920;
    height = 1080;
    return true;
}

void WeightedBlendedOIT::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionGL4();
}

void WeightedBlendedOIT::initUI()
{
    // sample apps automatically have a tweakbar they can use.
    if (mTweakBar)
    {
        mTweakBar->addValue("Opacity:", m_opacity, 0.0f, 1.0f, 0.05f);
        mTweakBar->addValue("Weight Parameter:", m_weightParameter, 0.1f, 1.0f, 0.05f);
        mTweakBar->addEnum("Model:", m_modelID, MODEL_OPTIONS, MODELS_COUNT);
        mTweakBar->addEnum("OIT Method:", m_mode, ALGORITHM_OPTIONS, MODE_COUNT);
    }

    // UI elements for displaying pass statistics
    if (mFPSText)
    {
        NvUIRect tr;
        mFPSText->GetScreenRect(tr); // base off of fps element.
        m_statsText = new NvUIText("", NvUIFontFamily::SANS, (mFPSText->GetFontSize()*2)/3, NvUITextAlign::RIGHT);
        m_statsText->SetColor(NV_PACKED_COLOR(0x30,0xD0,0xD0,0xB0));
        m_statsText->SetShadow();
        mUIWindow->Add(m_statsText, tr.left, tr.top+tr.height+8);
    }
}

void WeightedBlendedOIT::initRendering(void)
{
    NvAssetLoaderAddSearchPath("gl4-kepler/WeightedBlendedOIT");

    if(!requireMinAPIVersion(NvGLAPIVersionGL4_4(), true))
        return;

    if(!requireExtension("GL_ARB_texture_rectangle", true))
        return;

    // Allocate render targets first
    InitDepthPeelingRenderTargets();
    InitAccumulationRenderTargets();
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());

    BuildShaders();

    m_models[BUNNY_MODEL] = LoadModel("models/bunny.obj");
    m_models[DRAGON_MODEL] = LoadModel("models/dragon.obj");
    m_modelID = BUNNY_MODEL;

    glDisable(GL_CULL_FACE);

    glGenQueries(1, &m_queryId);

#if ENABLE_GPU_TIMERS
    for (uint32_t i = 0; i < TIMER_COUNT; ++i)
    {
        m_timers[i].init();
    }
#endif

    CHECK_GL_ERROR();
}

void WeightedBlendedOIT::reshape(int32_t width, int32_t height)
{
    if (m_imageWidth != width || m_imageHeight != height)
    {
        m_imageWidth = width;
        m_imageHeight = height;

        DeleteDepthPeelingRenderTargets();
        InitDepthPeelingRenderTargets();

        DeleteAccumulationRenderTargets();
        InitAccumulationRenderTargets();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());

    glViewport(0, 0, m_imageWidth, m_imageHeight);

    CHECK_GL_ERROR();
}

void WeightedBlendedOIT::draw(void)
{
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_numGeoPasses = 0;

    nv::matrix4f proj;
    nv::perspective(proj, 45.0f, (GLfloat)m_width / (GLfloat)m_height, 0.1f, 10.0f);
    m_MVP = proj * m_transformer->getModelViewMat();

    m_normalMat = m_transformer->getRotationMat();

    switch (m_mode)
    {
        case DEPTH_PEELING_MODE:
            RenderFrontToBackPeeling();
            break;
        case WEIGHTED_BLENDED_MODE:
            RenderWeightedBlendedOIT();
            break;
    }

#if ENABLE_GPU_TIMERS
    if (m_statsText)
    {
        static float gpuTimesMS[TIMER_COUNT] = { 0.f };
        for (int32_t i = 0; i < TIMER_COUNT; ++i)
        {
            if (m_timers[i].getStartStopCycles() >= NUM_FRAMES_PER_GPU_TIME)
            {
                gpuTimesMS[i] = m_timers[i].getScaledCycles() / m_timers[i].getStartStopCycles();
                m_timers[i].reset();
            }
        }

        std::ostringstream s;
        s << "geometry: " << std::fixed << std::setprecision(2) << gpuTimesMS[TIMER_GEOMETRY] << " ms" << std::endl;
        s << "compositing: " << std::fixed << std::setprecision(2) << gpuTimesMS[TIMER_COMPOSITING] << " ms" << std::endl;
        m_statsText->SetString(s.str().c_str());
    }
#endif
}

//--------------------------------------------------------------------------
void WeightedBlendedOIT::InitDepthPeelingRenderTargets()
{
    glGenTextures(2, m_frontDepthTexId);
    glGenTextures(2, m_frontColorTexId);
    glGenFramebuffers(2, m_frontFboId);

    for (int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_RECTANGLE, m_frontDepthTexId[i]);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT32F_NV,
                     m_imageWidth, m_imageHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        glBindTexture(GL_TEXTURE_RECTANGLE, m_frontColorTexId[i]);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, m_imageWidth, m_imageHeight,
                     0, GL_RGBA, GL_FLOAT, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, m_frontFboId[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_TEXTURE_RECTANGLE, m_frontDepthTexId[i], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_TEXTURE_RECTANGLE, m_frontColorTexId[i], 0);
    }

    glGenTextures(1, &m_frontColorBlenderTexId);
    glBindTexture(GL_TEXTURE_RECTANGLE, m_frontColorBlenderTexId);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, m_imageWidth, m_imageHeight,
                 0, GL_RGBA, GL_FLOAT, 0);

    glGenFramebuffers(1, &m_frontColorBlenderFboId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_frontColorBlenderFboId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_TEXTURE_RECTANGLE, m_frontDepthTexId[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_RECTANGLE, m_frontColorBlenderTexId, 0);
    CHECK_GL_ERROR();
}

//--------------------------------------------------------------------------
void WeightedBlendedOIT::DeleteDepthPeelingRenderTargets()
{
    glDeleteFramebuffers(2, m_frontFboId);
    glDeleteFramebuffers(1, &m_frontColorBlenderFboId);
    glDeleteTextures(2, m_frontDepthTexId);
    glDeleteTextures(2, m_frontColorTexId);
    glDeleteTextures(1, &m_frontColorBlenderTexId);
}

//--------------------------------------------------------------------------
void WeightedBlendedOIT::InitAccumulationRenderTargets()
{
    glGenTextures(2, m_accumulationTexId);

    glBindTexture(GL_TEXTURE_RECTANGLE, m_accumulationTexId[0]);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA16F,
                 m_imageWidth, m_imageHeight, 0, GL_RGBA, GL_FLOAT, NULL);

    glBindTexture(GL_TEXTURE_RECTANGLE, m_accumulationTexId[1]);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R8,
                 m_imageWidth, m_imageHeight, 0, GL_RED, GL_FLOAT, NULL);

    glGenFramebuffers(1, &m_accumulationFboId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_accumulationFboId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_RECTANGLE, m_accumulationTexId[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                           GL_TEXTURE_RECTANGLE, m_accumulationTexId[1], 0);

    CHECK_GL_ERROR();
}

//--------------------------------------------------------------------------
void WeightedBlendedOIT::DeleteAccumulationRenderTargets()
{
    glDeleteFramebuffers(1, &m_accumulationFboId);
    glDeleteTextures(2, m_accumulationTexId);
}

//--------------------------------------------------------------------------
NvGLModel* WeightedBlendedOIT::LoadModel(const char *model_filename)
{
    // Load model data for scene geometry
    int32_t length;
    char *modelData = NvAssetLoaderRead(model_filename, length);
    NvGLModel* model = new NvGLModel();
    model->loadModelFromObjData(modelData);
    model->rescaleModel(1.0f);
    model->initBuffers();

    NvAssetLoaderFree(modelData);

    CHECK_GL_ERROR();
    return model;
}

//--------------------------------------------------------------------------
void WeightedBlendedOIT::DrawModel(NvGLSLProgram* shader)
{
    CHECK_GL_ERROR();
    shader->setUniformMatrix4fv("uNormalMatrix", m_normalMat._array);
    shader->setUniformMatrix4fv("uModelViewMatrix", m_MVP._array);
    m_models[m_modelID]->drawElements(0, 1);
    CHECK_GL_ERROR();

    m_numGeoPasses++;
}

//--------------------------------------------------------------------------
void WeightedBlendedOIT::BuildShaders()
{
    NvGLSLProgram::setGlobalShaderHeader("#version 440\n");

    printf("\nloading shaders...\n");

    m_shaderPeelingBlend = NvGLSLProgram::createFromFiles("shaders/base_vertex.glsl",
        "shaders/front_peeling_blend_fragment.glsl");

    m_shaderPeelingFinal = NvGLSLProgram::createFromFiles("shaders/base_vertex.glsl",
        "shaders/front_peeling_final_fragment.glsl");

    m_shaderWeightedFinal = NvGLSLProgram::createFromFiles("shaders/base_vertex.glsl",
        "shaders/weighted_final_fragment.glsl");

    int32_t len;
    char* base_shade_vertex = NvAssetLoaderRead("shaders/base_shade_vertex.glsl", len);
    char* shade_fragment = NvAssetLoaderRead("shaders/shade_fragment.glsl", len);
    char* front_peeling_init_fragment = NvAssetLoaderRead("shaders/front_peeling_init_fragment.glsl", len);
    char* front_peeling_peel_fragment = NvAssetLoaderRead("shaders/front_peeling_peel_fragment.glsl", len);
    char* weighted_blend_fragment = NvAssetLoaderRead("shaders/weighted_blend_fragment.glsl", len);

    const char* vert[1];
    const char* frag[2];
    vert[0] = base_shade_vertex;
    frag[0] = shade_fragment;

    frag[1] = front_peeling_init_fragment;
    m_shaderPeelingInit = NvGLSLProgram::createFromStrings(vert, 1, frag, 2);
    NV_ASSERT(m_shaderPeelingInit);

    frag[1] = front_peeling_peel_fragment;
    m_shaderPeelingPeel = NvGLSLProgram::createFromStrings(vert, 1, frag, 2);
    NV_ASSERT(m_shaderPeelingPeel);

    frag[1] = weighted_blend_fragment;
    m_shaderWeightedBlend = NvGLSLProgram::createFromStrings(vert, 1, frag, 2);
    NV_ASSERT(m_shaderWeightedBlend);

    NvAssetLoaderFree(base_shade_vertex);
    NvAssetLoaderFree(shade_fragment);
    NvAssetLoaderFree(front_peeling_init_fragment);
    NvAssetLoaderFree(front_peeling_peel_fragment);
    NvAssetLoaderFree(weighted_blend_fragment);

    NvGLSLProgram::setGlobalShaderHeader(NULL);
}

//--------------------------------------------------------------------------
void WeightedBlendedOIT::DestroyShaders()
{
    delete m_shaderPeelingInit;
    delete m_shaderPeelingPeel;
    delete m_shaderPeelingBlend;
    delete m_shaderPeelingFinal;

    delete m_shaderWeightedBlend;
    delete m_shaderWeightedFinal;
}

//--------------------------------------------------------------------------
void WeightedBlendedOIT::ReloadShaders()
{
    DestroyShaders();
    BuildShaders();
}

//--------------------------------------------------------------------------
void WeightedBlendedOIT::RenderFrontToBackPeeling()
{
    {
#if ENABLE_GPU_TIMERS
        NvGPUTimerScope timer(&m_timers[TIMER_GEOMETRY]);
#endif

        // ---------------------------------------------------------------------
        // 1. Peel the first layer
        // ---------------------------------------------------------------------

        glBindFramebuffer(GL_FRAMEBUFFER, m_frontColorBlenderFboId);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);

        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);

        m_shaderPeelingInit->enable();
        m_shaderPeelingInit->setUniform1f("uAlpha", m_opacity);
        DrawModel(m_shaderPeelingInit);
        m_shaderPeelingInit->disable();

        CHECK_GL_ERROR();

        // ---------------------------------------------------------------------
        // 2. Depth Peeling + Blending
        // ---------------------------------------------------------------------

        for (int32_t layer = 1; layer < MAX_PEELED_LAYERS; layer++)
        {
            // ---------------------------------------------------------------------
            // 2.2. Peel the next depth layer
            // ---------------------------------------------------------------------

            int32_t currId = layer % 2;
            int32_t prevId = 1 - currId;

            glBindFramebuffer(GL_FRAMEBUFFER, m_frontFboId[currId]);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);

            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);

            glBeginQuery(GL_SAMPLES_PASSED, m_queryId);

            m_shaderPeelingPeel->enable();
            m_shaderPeelingPeel->bindTextureRect("DepthTex", 0, m_frontDepthTexId[prevId]);
            m_shaderPeelingPeel->setUniform1f("uAlpha", m_opacity);
            DrawModel(m_shaderPeelingPeel);
            m_shaderPeelingPeel->disable();

            glEndQuery(GL_SAMPLES_PASSED);

            CHECK_GL_ERROR();

            // ---------------------------------------------------------------------
            // 2.2. Blend the current layer
            // ---------------------------------------------------------------------

            glBindFramebuffer(GL_FRAMEBUFFER, m_frontColorBlenderFboId);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);

            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);

            // UNDER operator
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_DST_ALPHA, GL_ONE,
                                GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
        
            m_shaderPeelingBlend->enable();
            m_shaderPeelingBlend->bindTextureRect("TempTex", 0, m_frontColorTexId[currId]);
            RenderFullscreenQuad(m_shaderPeelingBlend);
            m_shaderPeelingBlend->disable();

            glDisable(GL_BLEND);

            CHECK_GL_ERROR();

            GLuint sample_count;
            glGetQueryObjectuiv(m_queryId, GL_QUERY_RESULT, &sample_count);
            if (sample_count == 0)
            {
                break;
            }
        }
    }

    {
#if ENABLE_GPU_TIMERS
        NvGPUTimerScope timer(&m_timers[TIMER_COMPOSITING]);
#endif

        // ---------------------------------------------------------------------
        // 3. Compositing Pass
        // ---------------------------------------------------------------------

        glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
        glDrawBuffer(GL_BACK);
        glDisable(GL_DEPTH_TEST);

        m_shaderPeelingFinal->enable();
        m_shaderPeelingFinal->setUniform3f("uBackgroundColor", 
            m_backgroundColor[0], m_backgroundColor[1], m_backgroundColor[2]);
        m_shaderPeelingFinal->bindTextureRect("ColorTex", 0, m_frontColorBlenderTexId);
        RenderFullscreenQuad(m_shaderPeelingFinal);
        m_shaderPeelingFinal->disable();

        CHECK_GL_ERROR();
    }
}

//--------------------------------------------------------------------------
void WeightedBlendedOIT::RenderWeightedBlendedOIT()
{
    glDisable(GL_DEPTH_TEST);

    {
#if ENABLE_GPU_TIMERS
        NvGPUTimerScope timer(&m_timers[TIMER_GEOMETRY]);
#endif

        // ---------------------------------------------------------------------
        // 1. Geometry pass
        // ---------------------------------------------------------------------

        glBindFramebuffer(GL_FRAMEBUFFER, m_accumulationFboId);

        const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, drawBuffers);

        // Render target 0 stores a sum (weighted RGBA colors). Clear it to 0.f.
        // Render target 1 stores a product (transmittances). Clear it to 1.f.
        float clearColorZero[4] = { 0.f, 0.f, 0.f, 0.f };
        float clearColorOne[4]  = { 1.f, 1.f, 1.f, 1.f };
        glClearBufferfv(GL_COLOR, 0, clearColorZero);
        glClearBufferfv(GL_COLOR, 1, clearColorOne);

        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunci(0, GL_ONE, GL_ONE);
        glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

        m_shaderWeightedBlend->enable();
        m_shaderWeightedBlend->setUniform1f("uAlpha", m_opacity);
        m_shaderWeightedBlend->setUniform1f("uDepthScale", m_weightParameter);
        DrawModel(m_shaderWeightedBlend);
        m_shaderWeightedBlend->disable();

        glDisable(GL_BLEND);

        CHECK_GL_ERROR();
    }

    {
#if ENABLE_GPU_TIMERS
        NvGPUTimerScope timer(&m_timers[TIMER_COMPOSITING]);
#endif

        // ---------------------------------------------------------------------
        // 2. Compositing pass
        // ---------------------------------------------------------------------

        glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
        glDrawBuffer(GL_BACK);

        m_shaderWeightedFinal->enable();
        m_shaderWeightedFinal->setUniform3f("uBackgroundColor", m_backgroundColor[0], m_backgroundColor[1], m_backgroundColor[2]);
        m_shaderWeightedFinal->bindTextureRect("ColorTex0", 0, m_accumulationTexId[0]);
        m_shaderWeightedFinal->bindTextureRect("ColorTex1", 1, m_accumulationTexId[1]);
        RenderFullscreenQuad(m_shaderWeightedFinal);
        m_shaderWeightedFinal->disable();

        CHECK_GL_ERROR();
    }
}

//--------------------------------------------------------------------------
void WeightedBlendedOIT::RenderFullscreenQuad(NvGLSLProgram* shader)
{
    shader->setUniformMatrix4fv("uModelViewMatrix", m_fullscreenMVP._array);
    NvDrawQuad(0);
}


NvAppBase* NvAppFactory() {
    return new WeightedBlendedOIT();
}
