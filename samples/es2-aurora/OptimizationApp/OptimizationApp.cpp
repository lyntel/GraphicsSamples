//----------------------------------------------------------------------------------
// File:        es2-aurora\OptimizationApp/OptimizationApp.cpp
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
#include "OptimizationApp.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvGLUtils/NvImageGL.h"
#include "NV/NvLogs.h"
#include "NvUI/NvTweakBar.h"

#include "SceneRenderer.h"
#include "AppExtensions.h"

void (KHRONOS_APIENTRY *glBlitFramebufferFunc) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

uint32_t gFloatTypeEnum;
uint32_t gLumaTypeEnum;

#define TO_RADIANS (NV_PI / 180.0f)

OptimizationApp::OptimizationApp() : 
    m_lightDirection(0.0f),
    m_center(0.0f),
    m_pausedByPerfHUD(false)
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();

    m_transformer->setRotationVec(nv::vec3f(11.405f * TO_RADIANS, 231.978f * TO_RADIANS, 0.0f));
    m_transformer->setTranslationVec(nv::vec3f(0.0f, -50.0f, 100.0f));
    m_transformer->setMaxTranslationVel(100.0f);
    m_transformer->setMotionMode(NvCameraMotionType::FIRST_PERSON);
}

OptimizationApp::~OptimizationApp()
{
    delete m_sceneRenderer;
    m_sceneRenderer = NULL;
    LOGI("OptimizationApp: destroyed\n");
}

void OptimizationApp::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionES3();
}

void OptimizationApp::initUI(void) {
    // sample apps automatically have a tweakbar they can use.
    if (mTweakBar) { // create our tweak ui
        // Show the app title
        mTweakBar->addLabel("Optimizations Sample App", true);

        mTweakBar->addPadding();
        mTweakBar->addValue("Draw particles:", m_sceneRenderer->getParticleParams()->render);
        mTweakBar->addValue("Use depth pre-pass:", m_sceneRenderer->getSceneParams()->useDepthPrepass);
        mTweakBar->addValue("Render low res scene:", m_sceneRenderer->getSceneParams()->renderLowResolution);
        mTweakBar->addValue("Render low res particles:", m_sceneRenderer->getParticleParams()->renderLowResolution);
        mTweakBar->addValue("Use cross-bilateral upsampling:", m_sceneRenderer->getUpsamplingParams()->useCrossBilateral);
    }

    // UI elements for displaying triangle statistics
    if (mFPSText) {
        NvUIRect tr;
        mFPSText->GetScreenRect(tr); // base off of fps element.
        m_timingStats = new NvUIText("Multi\nLine\nString", NvUIFontFamily::SANS, (mFPSText->GetFontSize()*2)/3, NvUITextAlign::RIGHT);
        m_timingStats->SetColor(NV_PACKED_COLOR(0x30,0xD0,0xD0,0xB0));
        m_timingStats->SetShadow();
        mUIWindow->Add(m_timingStats, tr.left, tr.top+tr.height+8);
    }
}

void OptimizationApp::initRendering(void) {
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);    

    NvAssetLoaderAddSearchPath("es2-aurora/OptimizationApp");

#ifdef GL_OES_texture_3D
    if (requireExtension("GL_EXT_framebuffer_blit", false)) {
        glBlitFramebufferFunc = (void (KHRONOS_APIENTRY *) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter))
            getGLContext()->getGLProcAddress("glBlitFramebufferEXT");
    } else if (getGLContext()->getConfiguration().apiVer != NvGLAPIVersionES2()) {
        glBlitFramebufferFunc = (void (KHRONOS_APIENTRY *) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter))
            getGLContext()->getGLProcAddress("glBlitFramebuffer");
    } else {
        glBlitFramebufferFunc = NULL;
    }
#else
    glBlitFramebufferFunc = glBlitFramebuffer;
#endif

    if (getGLContext()->getConfiguration().apiVer == NvGLAPIVersionES2()) {
        gFloatTypeEnum = 0x8D61; // GL_HALF_FLOAT_OES, not declared in GL
        gLumaTypeEnum = GL_LUMINANCE;
    } else {
        gFloatTypeEnum = GL_FLOAT;
        gLumaTypeEnum = 0x1903; // GL_RED, not declared in ES
    }

    m_sceneRenderer = new SceneRenderer(
        getGLContext()->getConfiguration().apiVer == NvGLAPIVersionES2());
    CHECK_GL_ERROR();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.0, 0.0, 1.0, 1.0);
}

void OptimizationApp::reshape(int32_t width, int32_t height)
{
    //setting the perspective projection matrix
    nv::perspective(m_projectionMatrix, EYE_FOVY_DEG * TO_RADIANS,    (float) width / (float) height, EYE_ZNEAR, EYE_ZFAR);
    m_sceneRenderer->setProjectionMatrix(m_projectionMatrix);

    m_sceneRenderer->reshapeWindow(width, height);

    glViewport( 0, 0, (GLint) width, (GLint) height );

    CHECK_GL_ERROR();
}

void OptimizationApp::updateViewDependentParams()
{
    //matrices for translation and rotation
    nv::matrix4f transM, rotM, lightRotM, lightTransM;
    
    //Rotation matrix
    rotM   = m_transformer->getRotationMat();
    transM = m_transformer->getTranslationMat();
    m_viewMatrix = rotM * transM;
    m_sceneRenderer->setEyeViewMatrix(m_viewMatrix);

    // code for computing light direction and lightViewMatrix
    {
        static nv::vec3f tempPoint = nv::normalize(nv::vec3f(-1, 1, -1));

        nv::vec3f front     = tempPoint - m_center;
        nv::vec3f axisRight = front.cross(nv::vec3f(0.0f, 1.0f, 0.0f));
        nv::vec3f up        = axisRight.cross(front);

        m_lightViewMatrix = nv::lookAt(m_lightViewMatrix, tempPoint, m_center, up);
        m_lightDirection  = nv::normalize<float>(m_viewMatrix * nv::vec4f(tempPoint.x, tempPoint.y, tempPoint.z, 0.0));

        const nv::vec3f tmpDir(m_lightDirection[0], m_lightDirection[1], m_lightDirection[2]);
        m_sceneRenderer->setLightDirection(tmpDir);
    }
}

void OptimizationApp::draw(void)
{
    GLuint prevFBO = 0;
    // Enum has MANY names based on extension/version
    // but they all map to 0x8CA6
    glGetIntegerv(0x8CA6, (GLint*)&prevFBO);
    
    m_sceneRenderer->getSceneFBOParams()->particleDownsample = 
        m_sceneRenderer->getParticleParams()->renderLowResolution ? 2.0f : 1.0f;
    m_sceneRenderer->getSceneFBOParams()->sceneDownsample = 
        m_sceneRenderer->getSceneParams()->renderLowResolution ? 2.0f : 1.0f;

    m_sceneRenderer->updateFrame(getFrameDeltaTime());

    // To maintain correct rendering of the blur we have to detect when we've been paused by PerfHUD.
    // This logic ensures that the time-dependent blur remains when the frame debugger is activated.
    m_pausedByPerfHUD = (getFrameDeltaTime() == 0.0f);

    // WARNING!!!!!  This is NOT a great idea for perf, but if we cannot disable
    // vsync, then the CPU stats will be wrong (because some GL call will block on last
    // frame's vsync)
    if (!getGLContext()->setSwapInterval(0))
        glFlush();

    if (!m_pausedByPerfHUD)
        updateViewDependentParams();

    glViewport(0, 0, m_width, m_height);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_sceneRenderer->renderFrame();

    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(0, 0, m_sceneRenderer->getScreenWidth(), m_sceneRenderer->getScreenHeight());
    char buffer[1024];
    if (m_sceneRenderer->stats(buffer, 1024)) {
        m_timingStats->SetString(buffer);
    }
}

NvAppBase* NvAppFactory() {
    return new OptimizationApp();
}
