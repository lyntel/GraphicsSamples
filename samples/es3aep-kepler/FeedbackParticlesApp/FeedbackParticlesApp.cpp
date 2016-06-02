//----------------------------------------------------------------------------------
// File:        es3aep-kepler\FeedbackParticlesApp/FeedbackParticlesApp.cpp
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
//------------------------------------------------------------------------------
//
#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <NvAppBase/NvFramerateCounter.h>
#include <NV/NvStopWatch.h>
#include <NvAssetLoader/NvAssetLoader.h>
#include <NvAppBase/NvInputTransformer.h>
#include <NvUI/NvTweakBar.h>
#include <NV/NvLogs.h>
#include "FeedbackParticlesApp.h"
#include "Utils.h"


//------------------------------------------------------------------------------
//
FeedbackParticlesApp::FeedbackParticlesApp() 
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}


//------------------------------------------------------------------------------
//
FeedbackParticlesApp::~FeedbackParticlesApp()
{
    LOGI("FeedbackParticlesApp: destroyed\n");
}


//------------------------------------------------------------------------------
//
void FeedbackParticlesApp::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits    = 24; 
    config.stencilBits  = 0; 
    config.apiVer       = NvGLAPIVersionGL4();
}


//------------------------------------------------------------------------------
//
void FeedbackParticlesApp::initRendering(void) 
{
    if (!requireMinAPIVersion(NvGLAPIVersionGL4())) 
        return;

    m_transformer->setTranslationVec(nv::vec3f(0.0f,0.0f,-3.0f));

    NvAssetLoaderAddSearchPath("es3aep-kepler/FeedbackParticlesApp");
    m_scene.init();
}


//------------------------------------------------------------------------------
//
void FeedbackParticlesApp::reshape(int32_t width, int32_t height)
{
    glViewport(0,0,(GLint)width,(GLint)height);

    m_width  = width;
    m_height = height;

    nv::matrix4f proj;
    nv::perspective(proj,NV_PI/3.0f,(float)m_width/(float)m_height,1.0f,150.0f);
    m_scene.setProjection(proj);
}


//------------------------------------------------------------------------------
//
void FeedbackParticlesApp::draw(void)
{
    m_scene.setView(m_transformer->getModelViewMat());
    m_scene.update(getFrameDeltaTime());
    m_scene.draw();

    if (mTweakBar)
        mTweakBar->syncValues();

    if (m_countText)
        m_countText->SetValue(m_scene.getParticlesCount());
}


//------------------------------------------------------------------------------
//
void FeedbackParticlesApp::initUI(void)
{
    if (mTweakBar)
    {
        ParticleSystem::Parameters& params = m_scene.getParticleParamsRef();

        mTweakBar->addValue("Stop simulation",m_scene.m_isSimulationStopped);
        mTweakBar->addValue("Freeze emitters",params.freezeEmitters);
        mTweakBar->addValue("Freeze colliders",params.freezeColliders);
        mTweakBar->addValue("Remove noise",params.removeNoise);
        mTweakBar->addValue("Use color",params.useColors);
        mTweakBar->addPadding();
        mTweakBar->addValue("Emitters count",params.emittersCount,1,32,1);
        mTweakBar->addValue("Emit period (s)",params.emitPeriod,0.f,1.f,0.01f);
        mTweakBar->addValue("Emit count",params.emitCount,0,100,1);
        mTweakBar->addValue("Particle lifetime (s)",params.particleLifetime,0.f,20.f,1.f);
        mTweakBar->addPadding();
        mTweakBar->addValue("Billboard size",params.billboardSize,0.f,0.1f,0.01f);
        mTweakBar->addValue("Velocity scale",params.velocityScale,0.f,4.0f,0.25f);
        mTweakBar->addPadding();

        mTweakBar->addValue("Reset",m_scene.m_reset, true);
    }

    // statistics
    if (mFPSText) {
        NvUIRect tr;
        mFPSText->GetScreenRect(tr);
        m_countText = new NvUIValueText("Particles", NvUIFontFamily::SANS, mFPSText->GetFontSize(), NvUITextAlign::RIGHT,
                                        0, NvUITextAlign::RIGHT);
        m_countText->SetColor(NV_PACKED_COLOR(0x30, 0xD0, 0xD0, 0xB0));
        m_countText->SetShadow();
        mUIWindow->Add(m_countText, tr.left, tr.top+tr.height+8);
    }
}


//------------------------------------------------------------------------------
//
void FeedbackParticlesApp::shutdownRendering(void)
{
    // If we still have a bound context, then it makes sense to delete GL resources
    if (getPlatformContext()->isContextBound())
        m_scene.release();
}


//------------------------------------------------------------------------------
//
NvAppBase* NvAppFactory() 
{
#if defined(_DEBUG) && defined(WIN32)
    _CrtSetDbgFlag( _CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF );
//    _CrtSetBreakAlloc(766);
#endif

    return new FeedbackParticlesApp();
}
