//----------------------------------------------------------------------------------
// File:        gl4-maxwell\CascadedShadowMapping/CascadedShadowMapping.cpp
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
#include "CascadedShadowMapping.h"
#include "CascadedShadowMappingRenderer.h"
#include <NV/NvLogs.h>
#include <NvAppBase/NvFramerateCounter.h>
#include <NvUI/NvTweakBar.h>
#include <NvUI/NvTweakVar.h>

struct UIReactionIDs {
    enum Enum
    {
        Method       = 0x10000001,
        SegmentCount = 0x10000002,
    };
};

static void printGLString(const char *name, GLenum s) {
    char *v = (char *)glGetString(s);
    LOGI("GL %s: %s\n", name, v);
}

CascadedShadowMapping::CascadedShadowMapping()
    : m_renderer(new CascadedShadowMappingRenderer(*this, *m_transformer))
    , m_method(ShadowMapMethod::GsNoCull)
    , m_frustumSegmentCount(4)
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
    m_renderer->setShadowMapMethod(static_cast<ShadowMapMethod::Enum>(m_method));
    m_renderer->setFrustumSegmentCount(m_frustumSegmentCount);
}

CascadedShadowMapping::~CascadedShadowMapping() {
    delete m_renderer;
    m_renderer = nullptr;
}

void CascadedShadowMapping::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionGL4();
}

void CascadedShadowMapping::initRendering(void) {
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    GLint depthBits;
    glGetIntegerv(GL_DEPTH_BITS, &depthBits);
    LOGI("depth bits = %d\n", depthBits);

    if (!requireMinAPIVersion(NvGLAPIVersionGL4_3()))
        return;

    if (!requireExtension("GL_NV_viewport_array2"))
        return;

    if (!requireExtension("GL_NV_geometry_shader_passthrough"))
        return;

    m_renderer->initRendering();

    CHECK_GL_ERROR();
}

void CascadedShadowMapping::initUI(void) {
    if (mTweakBar)
    {
        NvTweakEnum<uint32_t> shadowMapMethods[] =
        {
            { "Normal GS", ShadowMapMethod::GsNoCull },
            { "Normal GS With Viewport Culling", ShadowMapMethod::GsCull },
            { "Viewport Multicast With Culling", ShadowMapMethod::MulticastCull },
            { "Multicast with Fast GS and Culling", ShadowMapMethod::FgsMulticastCull },
            { "VS Only With Multicast", ShadowMapMethod::VsOnlyMulticast }
        };
        mTweakBar->addEnum("Shadow Map Method", m_method, shadowMapMethods, TWEAKENUM_ARRAYSIZE(shadowMapMethods), UIReactionIDs::Method);
        mTweakBar->addValue("Number of Frustum Segments", m_frustumSegmentCount, 1, MAX_CAMERA_FRUSTUM_SPLIT_COUNT, 1, UIReactionIDs::SegmentCount);
    }

    mFramerate->setMaxReportRate(0.25f);
    mFramerate->setReportFrames(5);
    getAppContext()->setSwapInterval(0);
    CHECK_GL_ERROR();
}

NvUIEventResponse CascadedShadowMapping::handleReaction(const NvUIReaction &react)
{
    switch (react.code) {
    case UIReactionIDs::Method:
        m_renderer->setShadowMapMethod(static_cast<ShadowMapMethod::Enum>(m_method));
        break;
    case UIReactionIDs::SegmentCount:
        m_renderer->setFrustumSegmentCount(m_frustumSegmentCount);
    default:
        return nvuiEventNotHandled;
    }

    return nvuiEventHandled;
}

void CascadedShadowMapping::draw(void) {
    float dt = getFrameDeltaTime();
    m_renderer->render(dt);
    CHECK_GL_ERROR();
}

void CascadedShadowMapping::reshape(int32_t width, int32_t height) {
    m_renderer->reshape(width, height);
    CHECK_GL_ERROR();
}

void CascadedShadowMapping::shutdownRendering(void) {
    m_renderer->shutdownRendering();
    CHECK_GL_ERROR();
}
