//----------------------------------------------------------------------------------
// File:        gl4-kepler\DeferredShadingMSAA/DeferredShadingMSAA.cpp
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
#include "DeferredShadingMSAA.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NvGLUtils/NvModelGL.h"
#include "NvModel/NvModel.h"
#include "NvGLUtils/NvShapesGL.h"
#include "NvModel/NvModelExt.h"
#include "NvGLUtils/NvModelExtGL.h"
#include "NvAppBase/NvInputTransformer.h"
#include <sstream>
#include <iomanip>

#define NUM_FRAMES_PER_GPU_TIME 30

// Sky color
const static float g_sky[3] = { 0.1, 0.3, 0.7 };

// The sample has a dropdown menu in the TweakBar that allows the user to
// select an approach to use to detect complexity.
// These are the options in that menu (for more details, please see the
// sample documentation)
static NvTweakEnum<uint32_t> APPROACH_OPTIONS[] =
{
    { "No MSAA", DeferredShadingMSAA::APPROACH_NOMSAA },
    { "Coverage Mask", DeferredShadingMSAA::APPROACH_COVERAGEMASK },
    { "Sample Discontinuity", DeferredShadingMSAA::APPROACH_DISCONTINUITY }
};

// The sample has a dropdown menu in the TweakBar that allows the user to
// select a BRDF to use in the lighting computations.
// These are the options in that menu (for more details, please see the
// sample documentation)
static NvTweakEnum<uint32_t> BRDF_OPTIONS[] =
{
    { "Lambert Diffuse", DeferredShadingMSAA::BRDF_LAMBERT},
    { "Oren Nayar Diffuse", DeferredShadingMSAA::BRDF_ORENNAYAR }
};

DeferredShadingMSAA::DeferredShadingMSAA() :
    m_modelID(0),
    m_queryId(0),
    m_imageWidth(0),
    m_imageHeight(0),
    m_iMSAACount(4),
    m_approach(APPROACH_COVERAGEMASK),
    m_brdf(BRDF_LAMBERT),
    m_currentRTMode(RTMODE_NONE),
    m_bAdaptiveShading(false),
    m_bMarkComplexPixels(false),
    m_bSeparateComplexPass(true),
    m_bUsePerSamplePixelShader(false),
    m_bUseExtendedModelLoader(true),
    m_backgroundColor(g_sky),
    m_shaderFillGBuffer(NULL),
    m_shaderFillGBuffer_NoMSAA(NULL),
    m_shaderMaskGeneration(NULL),
    m_shaderLighting(NULL),
    m_shaderLightingPerSample(NULL),
    m_shaderLightingMarkComplex(NULL),
    m_shaderLighting_NoMSAA(NULL),
    m_texGBufferFboId(0),
    m_texGBufResolved2(0),
    m_texGBufResolved2FboId(0),
    m_texDepthStencilBuffer(0),
    m_texOffscreenMSAA(0),
    m_texOffscreenMSAAFboId(0),
    m_lightingAccumulationBufferFboId(0),
    m_statsText(NULL),
    m_pUIApproachVar(NULL),
    m_pUIBRDFVar(NULL),
    m_pUIAdaptiveShadingVar(NULL),
    m_pUIMarkComplexPixelsVar(NULL),
    m_pUISeparateComplexPassVar(NULL),
    m_pUIPerSampleShadingVar(NULL)
{
    m_texGBuffer[0] = 0;
    m_texGBuffer[1] = 0;
    m_texGBuffer[2] = 0;
    m_transformer->setTranslationVec(nv::vec3f(15.0f, 0.0f, 0.0f));
    m_transformer->setRotationVec(nv::vec3f(0.0, 1.57, 0.0));
    m_transformer->setMotionMode(NvCameraMotionType::FIRST_PERSON);

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

DeferredShadingMSAA::~DeferredShadingMSAA()
{
    LOGI("DeferredShadingMSAA: destroyed\n");
}

bool DeferredShadingMSAA::getRequestedWindowSize(int32_t& width, int32_t& height)
{
    width = 1280;
    height = 1024;
    return true;
}

NvUIEventResponse DeferredShadingMSAA::handleReaction(const NvUIReaction& react)
{
    // Simple input hook to ensure that all settings are compatible after
    // a user has chosen to change something.  We'll use a flag to indicate that
    // we actually modified something so that we can force the other UI elements
    // to update to match their new state before we return.
    bool bStateModified = false;

    switch (react.code)
    {
    case UIReactionIDs::UIApproach:
    {
        if (react.ival == APPROACH_NOMSAA)
        {
            // If we're switching to NoMSAA mode, we need to clear any incompatible states
            m_bAdaptiveShading = false;
            m_bMarkComplexPixels = false;
            m_bSeparateComplexPass = false;
            m_bUsePerSamplePixelShader = false;
            bStateModified = true;
        }
        break;
    }
    case UIReactionIDs::UIAdaptiveShading:
    {
        // Adaptive shading requires us to be doing some form of MSAA,
        // so if we're not, switch to a valid approach
        if (m_approach == APPROACH_NOMSAA)
        {
            m_approach = APPROACH_COVERAGEMASK;
            bStateModified = true;
        }
        if (m_bAdaptiveShading && m_bUsePerSamplePixelShader)
        {
            // If we're turning on adaptive shading, then also turn off
            // the per sample shading, as they are mutually exclusive
            m_bUsePerSamplePixelShader = false;
            bStateModified = true;
        }        
        break;
    }
    case UIReactionIDs::UIMarkComplexPixels:
    {
        // Marking complex pixels requires us to be doing some form
        // of MSAA, so if we're not, switch to a valid approach
        if (m_approach == APPROACH_NOMSAA)
        {
            m_approach = APPROACH_COVERAGEMASK;
            bStateModified = true;
        }
        break;
    }
    case UIReactionIDs::UISeparateComplexPass:
    {
        // Using a separate pass for complex pixles requires us to be doing
        // some form of MSAA, so if we're not, switch to a valid approach
        if (m_approach == APPROACH_NOMSAA)
        {
            m_approach = APPROACH_COVERAGEMASK;
            bStateModified = true;
        }
        if (!m_bSeparateComplexPass && m_bUsePerSamplePixelShader)
        {
            // If we're turning off the separate complex pass, then also turn off
            // the per sample shading, as it requires the separate pass
            m_bUsePerSamplePixelShader = false;
            bStateModified = true;
        }
        break;
    }
    case UIReactionIDs::UIPerSampleShading:
    {
        // Adaptive shading requires us to be doing some form of MSAA,
        // so if we're not, switch to a valid approach
        if (m_approach == APPROACH_NOMSAA)
        {
            m_approach = APPROACH_COVERAGEMASK;
            bStateModified = true;
        }
        if (m_bUsePerSamplePixelShader)
        {
            if (!m_bSeparateComplexPass)
            {
                // If we're turning on per sample shading, then also turn on
                // the separate complex pass, as it requires the separate pass
                m_bSeparateComplexPass = true;
                bStateModified = true;
            }
            if (m_bAdaptiveShading)
            {
                // If we're turning on per sample shading, then turn off
                // adaptive shading, as they are mutually exclusive
                m_bAdaptiveShading = false;
                bStateModified = true;
            }
        }
        break;
    }
    }

    if (bStateModified)
    {
        mTweakBar->syncValues();
    }
    return nvuiEventNotHandled;
}

void DeferredShadingMSAA::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 0; 
    config.stencilBits = 8; 
    config.msaaSamples = 0;
    config.apiVer = NvGLAPIVersionGL4();
}

void DeferredShadingMSAA::initUI()
{
    // sample apps automatically have a tweakbar they can use.
    if (mTweakBar)
    {
        m_pUIApproachVar = mTweakBar->addEnum("Complex Pixel Detection Approach:", m_approach, APPROACH_OPTIONS, APPROACH_COUNT, UIReactionIDs::UIApproach);
        m_pUIBRDFVar = mTweakBar->addEnum("BRDF:", m_brdf, BRDF_OPTIONS, BRDF_COUNT, UIReactionIDs::UIBRDF);
        m_pUIAdaptiveShadingVar = mTweakBar->addValue("Adaptively Shade Complex Pixels", m_bAdaptiveShading, UIReactionIDs::UIAdaptiveShading);
        m_pUIMarkComplexPixelsVar = mTweakBar->addValue("Mark Complex Pixels", m_bMarkComplexPixels, UIReactionIDs::UIMarkComplexPixels);
        m_pUISeparateComplexPassVar = mTweakBar->addValue("Separate Complex Pixel Pass", m_bSeparateComplexPass, UIReactionIDs::UISeparateComplexPass);
        m_pUIPerSampleShadingVar = mTweakBar->addValue("Per-Sample Shading", m_bUsePerSamplePixelShader, UIReactionIDs::UIPerSampleShading);
        m_pUIExtendedModelLoaderVar = mTweakBar->addValue("Use Extended Model Loader", m_bUseExtendedModelLoader, UIReactionIDs::UIExtMeshLoader);
        mTweakBar->syncValues();
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

void DeferredShadingMSAA::initRendering(void)
{
    NvAssetLoaderAddSearchPath("gl4-kepler/DeferredShadingMSAA");
    NvAssetLoaderAddSearchPath("gl4-kepler/DeferredShadingMSAA/models");

    if(!requireMinAPIVersion(NvGLAPIVersionGL4(), true))
        return;

    // Require at least one of these multisample extensions
    if (!requireExtension("GL_NV_framebuffer_multisample", false))
    {
        if (!requireExtension("GL_ARB_multisample", true))
            return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());

    BuildShaders();

    // As a demo, we'll use both methods of loading meshes, and time each of them,
    // to illustrate the differences
    NvStopWatch* pStopwatch = createStopWatch();

    pStopwatch->start();
    m_models[SPONZA_MODEL] = LoadModel("models/sponza.obj");
    pStopwatch->stop();
    float time = pStopwatch->getTime();
    LOGI("Standard Model Loading Complete: %f seconds", time);

    pStopwatch->start();
    m_modelsExt[SPONZA_MODEL] = LoadModelExt("models/sponza.obj");
    pStopwatch->stop();
    time = pStopwatch->getTime();
    LOGI("Extended Model Loading Complete: %f seconds", time);

    m_modelID = SPONZA_MODEL;

    glDisable(GL_CULL_FACE);
    glGenQueries(1, &m_queryId);

#if ENABLE_GPU_TIMERS
    for (uint32_t i = 0; i < TIMER_COUNT; ++i)
    {
        m_timers[i].init();
    }
#endif
}

void DeferredShadingMSAA::reshape(int32_t width, int32_t height)
{
    // Avoid destroying render targets unnecessarily.  Only do it if our dimensions have actually changed.
    if (m_imageWidth != width || m_imageHeight != height)
    {
        m_imageWidth = width;
        m_imageHeight = height;

        // Destroy the current render targets and let them get recreated on our next draw
        DestroyRenderTargets();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());

    glViewport(0, 0, m_imageWidth, m_imageHeight);
}

void DeferredShadingMSAA::RenderToGBuffer()
{
#if ENABLE_GPU_TIMERS
        NvGPUTimerScope timer(&m_timers[TIMER_GEOMETRY]);
#endif
        // Set up our render targets as our g-buffer surfaces
        glBindFramebuffer(GL_FRAMEBUFFER, m_texGBufferFboId);

        const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glDrawBuffers(3, drawBuffers);

        float clearColorZero[4] = { 0.f, 0.f, 0.f, 0.f };
        glClearBufferfv(GL_COLOR, 0, clearColorZero);
        glClearBufferfv(GL_COLOR, 1, clearColorZero);
        glClearBufferfv(GL_COLOR, 2, clearColorZero);
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);

        m_shaderFillGBuffer->enable();

        // Pass our current matrices to the shader
        m_shaderFillGBuffer->setUniformMatrix4fv("uModelViewMatrix", m_transformer->getModelViewMat()._array);
        m_shaderFillGBuffer->setUniformMatrix4fv("uModelViewProjMatrix", m_MVP._array);
        m_shaderFillGBuffer->setUniformMatrix4fv("uNormalMatrix", m_normalMat._array);
        
        // set our sample mask to be all bits, based on the current MSAA level
        m_shaderFillGBuffer->setUniform1i("uSampleMask", (int)((1 << m_iMSAACount) - 1));

        DrawModel(m_shaderFillGBuffer);

        m_shaderFillGBuffer->disable();
}

void DeferredShadingMSAA::RenderToGBuffer_NoMSAA()
{
#if ENABLE_GPU_TIMERS
    NvGPUTimerScope timer(&m_timers[TIMER_GEOMETRY]);
#endif
    glBindFramebuffer(GL_FRAMEBUFFER, m_texGBufferFboId);

    const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);

    float clearColorZero[4] = { 0.f, 0.f, 0.f, 0.f };
    glClearBufferfv(GL_COLOR, 0, clearColorZero);
    glClearBufferfv(GL_COLOR, 1, clearColorZero);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    m_shaderFillGBuffer_NoMSAA->enable();

    // Pass our current matrices to the shader
    m_shaderFillGBuffer_NoMSAA->setUniformMatrix4fv("uModelViewMatrix", m_transformer->getModelViewMat()._array);
    m_shaderFillGBuffer_NoMSAA->setUniformMatrix4fv("uModelViewProjMatrix", m_MVP._array);
    m_shaderFillGBuffer_NoMSAA->setUniformMatrix4fv("uNormalMatrix", m_normalMat._array);

    DrawModel(m_shaderFillGBuffer_NoMSAA);

    m_shaderFillGBuffer_NoMSAA->disable();
}

void DeferredShadingMSAA::ResolveMSAAGBuffer()
{
    // Do a standard GL resolve of an MSAA surface to a non-MSAA surface by blitting
    // from one to the other
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_texGBufferFboId);
    glReadBuffer(GL_COLOR_ATTACHMENT1);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_texGBufResolved2FboId);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glBlitFramebuffer(0, 0, m_imageWidth, m_imageWidth, 0, 0, m_imageWidth, m_imageWidth, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void DeferredShadingMSAA::FillStencilBuffer()
{
    // We want to fill in our stencil buffer such that we will be able to 
    // use it in our 2-pass lighting modes.  We will fill it such that it
    // identifies complex pixels, so that we can use the faster shader on
    // simple pixels and only do the more expensive lighting on pixels
    // that need it.
    glBindFramebuffer(GL_FRAMEBUFFER, m_lightingAccumulationBufferFboId);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    glClear(GL_STENCIL_BUFFER_BIT);

    m_shaderMaskGeneration->enable();
 
    // Pass our current matrices to the shader
    m_shaderMaskGeneration->setUniformMatrix4fv("uModelViewMatrix", m_transformer->getModelViewMat()._array);

    // Use our G-Buffer as source textures
    m_shaderMaskGeneration->bindTexture2DMultisample("uTexGBuffer1", 0, m_texGBuffer[0]);
    m_shaderMaskGeneration->bindTexture2DMultisample("uTexGBuffer2", 1, m_texGBuffer[1]);
    m_shaderMaskGeneration->bindTexture2DMultisample("uTexCoverage", 2, m_texGBuffer[2]);

    // Provide the resolved non-MSAA color buffer so the shader can use it for certain lighting modes
    m_shaderMaskGeneration->bindTextureRect("uResolvedColorBuffer", 3, m_texGBufResolved2);
    m_shaderMaskGeneration->setUniform1i("uUseDiscontinuity", (m_approach == APPROACH_DISCONTINUITY));

    RenderFullscreenQuad(m_shaderMaskGeneration);

    m_shaderMaskGeneration->disable();
}

void DeferredShadingMSAA::PerformSinglePassLighting()
{
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());

    glDisable(GL_DEPTH_TEST);

    m_shaderLighting->enable();
    // Pass our current matrices to the shader
    m_shaderLighting->setUniformMatrix4fv("uProjViewMatrix", inverse(m_proj)._array);
    m_shaderLighting->setUniformMatrix4fv("uViewWorldMatrix", inverse(m_transformer->getModelViewMat())._array);

    // Pass all of the parameters that define our lighting and MSAA mode to the shader
    m_shaderLighting->setUniform1i("uSeparateEdgePass", m_bSeparateComplexPass);
    m_shaderLighting->setUniform1i("uSecondPass", GL_FALSE);
    m_shaderLighting->setUniform1i("uUseDiscontinuity", (m_approach == APPROACH_DISCONTINUITY));
    m_shaderLighting->setUniform1i("uAdaptiveShading", m_bAdaptiveShading);
    m_shaderLighting->setUniform1i("uShowComplexPixels", m_bMarkComplexPixels);
    m_shaderLighting->setUniform1i("uLightingModel", m_brdf);
    m_shaderLighting->setUniform1i("uMSAACount", m_iMSAACount);
    m_shaderLighting->setUniform1i("uSampleMask", (1 << m_iMSAACount) - 1);

    // Use our G-Buffer as source textures
    m_shaderLighting->bindTexture2DMultisample("uTexGBuffer1", 0, m_texGBuffer[0]);
    m_shaderLighting->bindTexture2DMultisample("uTexGBuffer2", 1, m_texGBuffer[1]);
    m_shaderLighting->bindTexture2DMultisample("uTexCoverage", 2, m_texGBuffer[2]);

    // Provide the resolved non-MSAA color buffer so the shader can use it for certain lighting modes
    m_shaderLighting->bindTextureRect("uResolvedColorBuffer", 3, m_texGBufResolved2);

    RenderFullscreenQuad(m_shaderLighting);

    m_shaderLighting->disable();
}

void DeferredShadingMSAA::PerformDualPassLighting_PerPixel()
{
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());

    glDisable(GL_DEPTH_TEST);

    m_shaderLighting->enable();
    // Pass our current matrices to the shader
    m_shaderLighting->setUniformMatrix4fv("uProjViewMatrix", inverse(m_proj)._array);
    m_shaderLighting->setUniformMatrix4fv("uViewWorldMatrix", inverse(m_transformer->getModelViewMat())._array);

    // Pass all of the parameters that define our lighting and MSAA mode to the shader
    m_shaderLighting->setUniform1i("uSeparateEdgePass", m_bSeparateComplexPass);
    m_shaderLighting->setUniform1i("uUseDiscontinuity", (m_approach == APPROACH_DISCONTINUITY));
    m_shaderLighting->setUniform1i("uAdaptiveShading", m_bAdaptiveShading);
    m_shaderLighting->setUniform1i("uShowComplexPixels", m_bMarkComplexPixels);
    m_shaderLighting->setUniform1i("uLightingModel", m_brdf);
    m_shaderLighting->setUniform1i("uMSAACount", m_iMSAACount);
    m_shaderLighting->setUniform1i("uSampleMask", (1 << m_iMSAACount) - 1);

    // Use our G-Buffer as source textures
    m_shaderLighting->bindTexture2DMultisample("uTexGBuffer1", 0, m_texGBuffer[0]);
    m_shaderLighting->bindTexture2DMultisample("uTexGBuffer2", 1, m_texGBuffer[1]);
    m_shaderLighting->bindTexture2DMultisample("uTexCoverage", 2, m_texGBuffer[2]);

    // Provide the resolved non-MSAA color buffer so the shader can use it for certain lighting modes
    m_shaderLighting->bindTextureRect("uResolvedColorBuffer", 3, m_texGBufResolved2);


    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0x00); // Disable writing to the stencil mask

    // Render First (Simple Pixel) Pass
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    m_shaderLighting->setUniform1i("uSecondPass", GL_FALSE);
    RenderFullscreenQuad(m_shaderLighting);

    // Render Second (Complex Pixel) Pass
    glStencilFunc(GL_EQUAL, 0, 0xFF);
    m_shaderLighting->setUniform1i("uSecondPass", GL_TRUE);
    RenderFullscreenQuad(m_shaderLighting);

    m_shaderLighting->disable();
}

void DeferredShadingMSAA::PerformDualPassLighting_PerSample()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_texOffscreenMSAAFboId);

    glDisable(GL_DEPTH_TEST);

    m_shaderLighting->enable();
    // Pass our current matrices to the shader
    m_shaderLighting->setUniformMatrix4fv("uProjViewMatrix", inverse(m_proj)._array);
    m_shaderLighting->setUniformMatrix4fv("uViewWorldMatrix", inverse(m_transformer->getModelViewMat())._array);

    // Pass all of the parameters that define our lighting and MSAA mode to the shader
    m_shaderLighting->setUniform1i("uSeparateEdgePass", m_bSeparateComplexPass);
    m_shaderLighting->setUniform1i("uSecondPass", GL_FALSE);
    m_shaderLighting->setUniform1i("uUseDiscontinuity", (m_approach == APPROACH_DISCONTINUITY));
    m_shaderLighting->setUniform1i("uAdaptiveShading", m_bAdaptiveShading);
    m_shaderLighting->setUniform1i("uShowComplexPixels", m_bMarkComplexPixels);
    m_shaderLighting->setUniform1i("uLightingModel", m_brdf);
    m_shaderLighting->setUniform1i("uMSAACount", m_iMSAACount);
    m_shaderLighting->setUniform1i("uSampleMask", (1 << m_iMSAACount) - 1);

    // Use our G-Buffer as source textures
    m_shaderLighting->bindTexture2DMultisample("uTexGBuffer1", 0, m_texGBuffer[0]);
    m_shaderLighting->bindTexture2DMultisample("uTexGBuffer2", 1, m_texGBuffer[1]);
    m_shaderLighting->bindTexture2DMultisample("uTexCoverage", 2, m_texGBuffer[2]);

    // Provide the resolved non-MSAA color buffer so the shader can use it for certain lighting modes
    m_shaderLighting->bindTextureRect("uResolvedColorBuffer", 3, m_texGBufResolved2);

    // Render First (Simple Pixel) Pass
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilMask(0x00); // Disable writing to the stencil mask

    RenderFullscreenQuad(m_shaderLighting);

    m_shaderLighting->disable();

    // Render Second (Complex Pixel) Pass
    glStencilFunc(GL_EQUAL, 0, 0xFF);

    if (m_bMarkComplexPixels)
    {
        m_shaderLightingMarkComplex->enable();
        RenderFullscreenQuad(m_shaderLightingMarkComplex);
        m_shaderLightingMarkComplex->disable();
    }
    else
    {
        m_shaderLightingPerSample->enable();
        m_shaderLightingPerSample->setUniformMatrix4fv("uProjViewMatrix", inverse(m_proj)._array);
        m_shaderLightingPerSample->setUniformMatrix4fv("uViewWorldMatrix", inverse(m_transformer->getModelViewMat())._array);
        m_shaderLightingPerSample->setUniform1i("uSecondPass", GL_TRUE);
        m_shaderLightingPerSample->setUniform1i("uLightingModel", m_brdf);

        m_shaderLightingPerSample->bindTexture2DMultisample("uTexGBuffer1", 0, m_texGBuffer[0]);
        m_shaderLightingPerSample->bindTexture2DMultisample("uTexGBuffer2", 1, m_texGBuffer[1]);
        m_shaderLightingPerSample->bindTexture2DMultisample("uTexCoverage", 2, m_texGBuffer[2]);
        m_shaderLightingPerSample->bindTextureRect("uResolvedColorBuffer", 3, m_texGBufResolved2);
        
        glEnable(GL_SAMPLE_SHADING);
        RenderFullscreenQuad(m_shaderLightingPerSample);
        glDisable(GL_SAMPLE_SHADING);

        m_shaderLightingPerSample->disable();
    }

    // Now resolve from our off-screen MSAA lighting accumulation buffer to our final, presentable framebuffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_texOffscreenMSAAFboId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, getMainFBO());

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glBlitFramebuffer(0, 0, m_imageWidth, m_imageWidth, 0, 0, m_imageWidth, m_imageWidth, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void DeferredShadingMSAA::PerformSinglePassLighting_NoMSAA()
{
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    m_shaderLighting_NoMSAA->enable();
    m_shaderLighting_NoMSAA->setUniformMatrix4fv("uProjViewMatrix", inverse(m_proj)._array);
    m_shaderLighting_NoMSAA->setUniformMatrix4fv("uViewWorldMatrix", inverse(m_transformer->getModelViewMat())._array);

    m_shaderLighting_NoMSAA->setUniform1i("uLightingModel", m_brdf);
    m_shaderLighting_NoMSAA->bindTextureRect("uTexGBuffer1", 0, m_texGBuffer[0]);
    m_shaderLighting_NoMSAA->bindTextureRect("uTexGBuffer2", 1, m_texGBuffer[1]);

    RenderFullscreenQuad(m_shaderLighting_NoMSAA);

    m_shaderLighting_NoMSAA->disable();
}

void DeferredShadingMSAA::draw(void)
{
    nv::perspective(m_proj, 45.0f, (GLfloat)m_width / (GLfloat)m_height, 0.1f, 100.0f);
    m_MVP = m_proj * m_transformer->getModelViewMat();
    m_normalMat = m_transformer->getRotationMat();

    if (!UpdateRenderTargetMode())
    {
        // We weren't able to update our render targets properly, so 
        // there's no reason to render anything.
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());

    glClearDepth(1.0f);
    glClearStencil(0);
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Render scene geometry to G-Buffer
    if (m_currentRTMode == RTMODE_NOMSAA)
    {
        RenderToGBuffer_NoMSAA();
    }
    else
    { 
        RenderToGBuffer();
    }

    // Resolve G-Buffer if we're using a technique that requires it
    if (m_approach == APPROACH_COVERAGEMASK)
    {
        ResolveMSAAGBuffer();
    }

    if (m_currentRTMode == RTMODE_NOMSAA)
    {
        PerformSinglePassLighting_NoMSAA();
    }
    else if (m_bSeparateComplexPass)
    {
        // Since we are doing two passes, we need a stencil buffer to identify 
        // which pixels to process as complex (edge) pixels
        FillStencilBuffer();

        if (m_bUsePerSamplePixelShader)
        {
            PerformDualPassLighting_PerSample();
        }
        else
        {
            PerformDualPassLighting_PerPixel();
        }
    }
    else
    {
        PerformSinglePassLighting();
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

bool DeferredShadingMSAA::UpdateRenderTargetMode()
{
    // This method can be called before we have a valid size for our render targets.
    // In that case, simply return and wait for a call with valid values.
    if (m_imageWidth == 0 || m_imageHeight == 0)
    {
        return false;
    }

    // Determine what frame buffer mode we need to be in, and if we're not
    // currently in that mode, destroy the current render targets and 
    // initialize the new ones.
    if (m_approach == APPROACH_NOMSAA)
    {
        if (m_currentRTMode != RTMODE_NOMSAA)
        {
            DestroyRenderTargets();
            InitRenderTargets_NoMSAA();
        }
    }
    else if (m_bUsePerSamplePixelShader)
    {
        if (m_currentRTMode != RTMODE_PERSAMPLE)
        {
            DestroyRenderTargets();
            InitRenderTargets_PerSampleShading();
        }
    }
    else
    {
        if (m_currentRTMode != RTMODE_PERPIXEL)
        {
            DestroyRenderTargets();
            InitRenderTargets_PerPixelShading();
        }
    }
    return true;
}

void DeferredShadingMSAA::DestroyRenderTargets()
{
    // Validating our render targets at the start of rendering each frame means that
    // we can always destroy them when circumstances change, such as a resize event,
    // and not have to immediately re-create them.  This helps with resize events 
    // that happen multiple times between draw calls, which would otherwise have us 
    // recreating surfaces that would just be destroyed again before being used.
    if (m_currentRTMode != RTMODE_NONE)
    {
        glDeleteFramebuffers(1, &m_texOffscreenMSAAFboId);
        m_texOffscreenMSAAFboId = 0;
        glDeleteFramebuffers(1, &m_texGBufResolved2FboId);
        m_texGBufResolved2FboId = 0;
        glDeleteFramebuffers(1, &m_texGBufferFboId);
        m_texGBufferFboId = 0;
        glDeleteTextures(1, &m_texOffscreenMSAA);
        m_texOffscreenMSAA = 0;
        glDeleteTextures(1, &m_texGBufResolved2);
        m_texGBufResolved2 = 0;
        m_lightingAccumulationBufferFboId = 0;
        glDeleteTextures(1, &m_texDepthStencilBuffer);
        m_texDepthStencilBuffer = 0;
        glDeleteTextures(3, m_texGBuffer);
        m_texGBuffer[0] = 0;
        m_texGBuffer[1] = 0;
        m_texGBuffer[2] = 0;
        m_currentRTMode = RTMODE_NONE;
    }
}

void DeferredShadingMSAA::InitRenderTargets_NoMSAA()
{
    glDisable(GL_MULTISAMPLE);

    // Create textures for the G-Buffer
    {
        glGenTextures(2, m_texGBuffer);
        // Ensure that the unused entry in our array is 0 so that it doesn't cause problems when deleting
        m_texGBuffer[2] = 0;

        // Buffer 0 holds Normals and Depth
        glBindTexture(GL_TEXTURE_RECTANGLE, m_texGBuffer[0]);
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA16F, m_imageWidth, m_imageHeight, 0, GL_RGBA, GL_FLOAT, 0);

        // Buffer 1 holds Color
        glBindTexture(GL_TEXTURE_RECTANGLE, m_texGBuffer[1]);
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA16F, m_imageWidth, m_imageHeight, 0, GL_RGBA, GL_FLOAT, 0);
    }

    // Create the Depth/Stencil
    glGenTextures(1, &m_texDepthStencilBuffer);
    glBindTexture(GL_TEXTURE_RECTANGLE, m_texDepthStencilBuffer);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH24_STENCIL8, m_imageWidth, m_imageHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);

    glBindTexture(GL_TEXTURE_RECTANGLE, 0);

    glGenFramebuffers(1, &m_texGBufferFboId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_texGBufferFboId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, m_texGBuffer[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_RECTANGLE, m_texGBuffer[1], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_RECTANGLE, m_texDepthStencilBuffer, 0);

    // Lighting will be done to the default frame buffer
    m_lightingAccumulationBufferFboId = 0;

    m_currentRTMode = RTMODE_NOMSAA;
}

void DeferredShadingMSAA::InitRenderTargets_PerPixelShading()
{
    glEnable(GL_MULTISAMPLE);

    // Create textures for the G-Buffer
    {
        glGenTextures(3, m_texGBuffer);

        // Buffer 0 holds Normals and Depth
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_texGBuffer[0]);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_iMSAACount, GL_RGBA16F, m_imageWidth, m_imageHeight, false);

        // Buffer 1 holds Color and a flag indicating Edge pixels
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_texGBuffer[1]);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_iMSAACount, GL_RGBA16F, m_imageWidth, m_imageHeight, false);

        // Buffer 2 holds a copy of the sample's Coverage Mask 
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_texGBuffer[2]);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_iMSAACount, GL_R8UI, m_imageWidth, m_imageHeight, false);
    }

    // Create the Depth/Stencil
    glGenTextures(1, &m_texDepthStencilBuffer);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_texDepthStencilBuffer);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_iMSAACount, GL_DEPTH24_STENCIL8, m_imageWidth, m_imageHeight, false);

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

    glGenFramebuffers(1, &m_texGBufferFboId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_texGBufferFboId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_texGBuffer[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, m_texGBuffer[1], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D_MULTISAMPLE, m_texGBuffer[2], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, m_texDepthStencilBuffer, 0);

    glGenTextures(1, &m_texGBufResolved2);
    glBindTexture(GL_TEXTURE_RECTANGLE, m_texGBufResolved2);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA16F, m_imageWidth, m_imageHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glGenFramebuffers(1, &m_texGBufResolved2FboId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_texGBufResolved2FboId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, m_texGBufResolved2, 0);

    // Lighting will be done to the default frame buffer
    m_lightingAccumulationBufferFboId = 0;

    m_currentRTMode = RTMODE_PERPIXEL;
}

void DeferredShadingMSAA::InitRenderTargets_PerSampleShading()
{
    glEnable(GL_MULTISAMPLE);

    // Create textures for the G-Buffer
    {
        glGenTextures(3, m_texGBuffer);

        // Buffer 0 holds Normals and Depth
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_texGBuffer[0]);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_iMSAACount, GL_RGBA16F, m_imageWidth, m_imageHeight, false);

        // Buffer 1 holds Color and a flag indicating Edge pixels
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_texGBuffer[1]);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_iMSAACount, GL_RGBA16F, m_imageWidth, m_imageHeight, false);

        // Buffer 2 holds a copy of the sample's Coverage Mask 
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_texGBuffer[2]);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_iMSAACount, GL_R8UI, m_imageWidth, m_imageHeight, false);
    }

    // Create the Depth/Stencil
    glGenTextures(1, &m_texDepthStencilBuffer);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_texDepthStencilBuffer);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_iMSAACount, GL_DEPTH24_STENCIL8, m_imageWidth, m_imageHeight, false);

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

    glGenFramebuffers(1, &m_texGBufferFboId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_texGBufferFboId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_texGBuffer[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, m_texGBuffer[1], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D_MULTISAMPLE, m_texGBuffer[2], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, m_texDepthStencilBuffer, 0);

    glGenTextures(1, &m_texGBufResolved2);
    glBindTexture(GL_TEXTURE_RECTANGLE, m_texGBufResolved2);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA16F, m_imageWidth, m_imageHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glGenFramebuffers(1, &m_texGBufResolved2FboId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_texGBufResolved2FboId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, m_texGBufResolved2, 0);

    // Create the off-screen MSAA surface for rendering the per-sample lighting pass into
    glGenTextures(1, &m_texOffscreenMSAA);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_texOffscreenMSAA);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_iMSAACount, GL_RGBA16F, m_imageWidth, m_imageHeight, false);
    glGenFramebuffers(1, &m_texOffscreenMSAAFboId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_texOffscreenMSAAFboId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_texOffscreenMSAA, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, m_texDepthStencilBuffer, 0);

    // Lighting will be done to the off-screen MSAA frame buffer
    m_lightingAccumulationBufferFboId = m_texOffscreenMSAAFboId;

    m_currentRTMode = RTMODE_PERSAMPLE;
}

//--------------------------------------------------------------------------
NvModelGL* DeferredShadingMSAA::LoadModel(const char *model_filename)
{
    // Load model data for scene geometry
    int32_t length;
    char *modelData = NvAssetLoaderRead(model_filename, length);
	NvModelGL* model = NvModelGL::CreateFromObj(
		(uint8_t *)modelData, 40.0f, true, false);

    NvAssetLoaderFree(modelData);

    return model;
}

class MSAALoader : public Nv::NvModelFileLoader
{
public:
	MSAALoader() {}
	virtual ~MSAALoader() {}
	virtual char* LoadDataFromFile(const char* fileName)
	{
		int32_t length;
		return NvAssetLoaderRead(fileName, length);
	}

	virtual void ReleaseData(char* pData)
	{
		NvAssetLoaderFree(pData);
	}
};

//--------------------------------------------------------------------------
Nv::NvModelExtGL* DeferredShadingMSAA::LoadModelExt(const char *model_filename)
{
	MSAALoader loader;
	Nv::NvModelExt::SetFileLoader(&loader);

	Nv::NvModelExt* pModel = Nv::NvModelExt::CreateFromObj(model_filename, 40.0f, true, true);

    Nv::NvModelExtGL* pGLModelExt = Nv::NvModelExtGL::Create(pModel);

    return pGLModelExt;
}

//--------------------------------------------------------------------------
void DeferredShadingMSAA::DrawModel(NvGLSLProgram* shader)
{
    shader->setUniformMatrix4fv("uNormalMatrix", m_normalMat._array);
    shader->setUniformMatrix4fv("uModelViewMatrix", m_MVP._array);
    if (m_bUseExtendedModelLoader)
    {
		Nv::NvModelExtGL* model = m_modelsExt[m_modelID];
		uint32_t numMeshes = model->GetMeshCount();
		for (uint32_t meshIndex = 0; meshIndex < numMeshes; ++meshIndex)
		{
			Nv::NvMeshExtGL* pMesh = model->GetMesh(meshIndex);
			uint32_t matId = pMesh->GetMaterialID();
			Nv::NvMaterialGL* pMat = model->GetMaterial(matId);

			uint32_t diffuseTextureIndex = pMat->m_diffuseTexture;

			// The only material property we really care about is our diffuse surface color
			shader->setUniform3fv("uDiffuseColor", (float*)(pMat->m_diffuse));

			pMesh->DrawElements(0, 0, 1, 2);
		}
	}
    else
    {
        m_models[m_modelID]->drawElements(0, 1);
    }
}

//--------------------------------------------------------------------------
void DeferredShadingMSAA::BuildShaders()
{
    NvGLSLProgram::setGlobalShaderHeader("#version 410\n");

    printf("\nloading shaders...\n");

    m_shaderFillGBuffer = NvGLSLProgram::createFromFiles("shaders/basicMesh_VS.glsl", "shaders/basicMesh_FS.glsl");
    m_shaderFillGBuffer_NoMSAA = NvGLSLProgram::createFromFiles("shaders/basicMesh_VS.glsl", "shaders/basicMesh_NoMSAA_FS.glsl");
    m_shaderMaskGeneration = NvGLSLProgram::createFromFiles("shaders/ComplexMask_VS.glsl", "shaders/ComplexMask_FS.glsl");

    int32_t len;
    char* lightingVSSrc = NvAssetLoaderRead("shaders/Lighting_VS.glsl", len);

    char* lightingFSSrc[2];
    lightingFSSrc[0] = NvAssetLoaderRead("shaders/Lighting_FS_Shared.h", len);
    lightingFSSrc[1] = NvAssetLoaderRead("shaders/Lighting_FS.glsl", len);
    m_shaderLighting = NvGLSLProgram::createFromStrings((const char**)(&lightingVSSrc), 1, (const char**)lightingFSSrc, 2, false);

    lightingFSSrc[1] = NvAssetLoaderRead("shaders/Lighting_PerSample_FS.glsl", len);
    m_shaderLightingPerSample = NvGLSLProgram::createFromStrings((const char**)(&lightingVSSrc), 1, (const char**)lightingFSSrc, 2, false);
    
    lightingFSSrc[1] = NvAssetLoaderRead("shaders/Lighting_NoMSAA_FS.glsl", len);
    m_shaderLighting_NoMSAA = NvGLSLProgram::createFromStrings((const char**)(&lightingVSSrc), 1, (const char**)lightingFSSrc, 2, false);

    m_shaderLightingMarkComplex = NvGLSLProgram::createFromFiles("shaders/Lighting_VS.glsl", "shaders/MarkComplex_PerSample_FS.glsl");

    NvGLSLProgram::setGlobalShaderHeader(NULL);
}

//--------------------------------------------------------------------------
void DeferredShadingMSAA::DestroyShaders()
{
    delete m_shaderFillGBuffer;
    m_shaderFillGBuffer = NULL;

    delete m_shaderFillGBuffer_NoMSAA;
    m_shaderFillGBuffer_NoMSAA = NULL;
    
    delete m_shaderMaskGeneration;
    m_shaderMaskGeneration = NULL;

    delete m_shaderLighting;
    m_shaderLighting = NULL;

    delete m_shaderLightingPerSample;
    m_shaderLightingPerSample = NULL;

    delete m_shaderLighting_NoMSAA;
    m_shaderLighting_NoMSAA = NULL;

    delete m_shaderLightingMarkComplex;
    m_shaderLightingMarkComplex = NULL;
}

//--------------------------------------------------------------------------
void DeferredShadingMSAA::ReloadShaders()
{
    DestroyShaders();
    BuildShaders();
}

//--------------------------------------------------------------------------
void DeferredShadingMSAA::RenderFullscreenQuad(NvGLSLProgram* shader)
{
    shader->setUniformMatrix4fv("uModelViewMatrix", m_fullscreenMVP._array);
    NvDrawQuadGL(0);
}

NvAppBase* NvAppFactory() 
{
    return new DeferredShadingMSAA();
}
