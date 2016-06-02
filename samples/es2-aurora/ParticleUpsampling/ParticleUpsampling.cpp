//----------------------------------------------------------------------------------
// File:        es2-aurora\ParticleUpsampling/ParticleUpsampling.cpp
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
#include "ParticleUpsampling.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"

#include "SceneRenderer.h"

ParticleUpsampling::ParticleUpsampling()
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

ParticleUpsampling::~ParticleUpsampling()
{
    LOGI("ParticleUpsampling: destroyed\n");
}

void ParticleUpsampling::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionGL4();
}

enum TweakBarActionCodes
{
    REACT_UPDATE_SCREEN_BUFFERS = 1,
    REACT_UPDATE_LIGHT_BUFFERS,
};

void ParticleUpsampling::initUI(void)
{
    if (mTweakBar)
    {
        NvTweakVarBase *var;

        var = mTweakBar->addValue("Shadow Render", m_sceneRenderer->getParticleParams()->renderShadows);
        addTweakKeyBind(var, NvKey::K_S);
        addTweakButtonBind(var, NvGamepad::BUTTON_X);
        var = mTweakBar->addValue("Model Render", m_sceneRenderer->getSceneParams()->drawModel);
        addTweakKeyBind(var, NvKey::K_M);
        addTweakButtonBind(var, NvGamepad::BUTTON_X);
        var = mTweakBar->addValue("Use Depth Prepass", m_sceneRenderer->getSceneParams()->useDepthPrepass);
        addTweakKeyBind(var, NvKey::K_U);

        mTweakBar->addPadding();
        NvTweakEnum<uint32_t> shadowSliceModes[] = {
            {"16",  16},
            {"32",  32},
            {"64",  64}
        };
        var = mTweakBar->addEnum("Num Shadow Slices", m_sceneRenderer->getParticleParams()->numSlices, shadowSliceModes, TWEAKENUM_ARRAYSIZE(shadowSliceModes));
        addTweakKeyBind(var, NvKey::K_N);

        mTweakBar->addPadding();
        NvTweakEnum<uint32_t> particleDownsampleModes[] = {
            {"Full-Res",    1},
            {"Half-Res",    2},
            {"Quarter-Res", 4}
        };
        var = mTweakBar->addEnum("Particle Downsample Mode", m_sceneRenderer->getSceneFBOParams()->particleDownsample, particleDownsampleModes, TWEAKENUM_ARRAYSIZE(particleDownsampleModes), REACT_UPDATE_SCREEN_BUFFERS);
        addTweakKeyBind(var, NvKey::K_P);
        addTweakButtonBind(var, NvGamepad::BUTTON_RIGHT_SHOULDER);

        NvTweakEnum<uint32_t> lightBufferSizeModes[] = {
            {"64x64",   64},
            {"128x128",  128},
            {"256x256",  256}
        };
        var = mTweakBar->addEnum("Light Buffer Size", m_sceneRenderer->getSceneFBOParams()->lightBufferSize, lightBufferSizeModes, TWEAKENUM_ARRAYSIZE(lightBufferSizeModes), REACT_UPDATE_LIGHT_BUFFERS);
        addTweakKeyBind(var, NvKey::K_L);
    }
}

NvUIEventResponse ParticleUpsampling::handleReaction(const NvUIReaction& react)
{
    switch(react.code)
    {
    case REACT_UPDATE_SCREEN_BUFFERS:
        m_sceneRenderer->createScreenBuffers();
        return nvuiEventHandled;
    case REACT_UPDATE_LIGHT_BUFFERS:
        m_sceneRenderer->createLightBuffer();
        return nvuiEventHandled;
    default:
        break;
    }
    return nvuiEventNotHandled;
}

void ParticleUpsampling::initRendering(void)
{
    //glClearColor(0.5f, 0.5f, 0.5f, 1.0f); 
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);    

    NvAssetLoaderAddSearchPath("es2-aurora/ParticleUpsampling");

    m_sceneRenderer = new SceneRenderer(requireMinAPIVersion(NvGLAPIVersionGL4(), false));

    CHECK_GL_ERROR();
}

void ParticleUpsampling::reshape(int32_t width, int32_t height)
{
    glViewport( 0, 0, (GLint) width, (GLint) height );

    m_sceneRenderer->reshapeWindow(width, height);

    CHECK_GL_ERROR();
}

void ParticleUpsampling::draw(void)
{
    GLuint prevFBO = 0;
    // Enum has MANY names based on extension/version
    // but they all map to 0x8CA6
    glGetIntegerv(0x8CA6, (GLint*)&prevFBO);
    CHECK_GL_ERROR();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    matrix4f rotationMatrix, translationMatrix;
    nv::rotationY(rotationMatrix, m_transformer->getRotationVec().y);
    nv::translation(translationMatrix, 0.f, 0.f, -5.f);
    translationMatrix.set_scale(vec3f(-1.0, 1.0, -1.0));
    m_sceneRenderer->setEyeViewMatrix(translationMatrix * rotationMatrix);

    m_sceneRenderer->renderFrame();

    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(0, 0, m_width, m_height);
}


NvAppBase* NvAppFactory() {
    return new ParticleUpsampling();
}
