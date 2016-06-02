//----------------------------------------------------------------------------------
// File:        tutorial\basic/Basic.cpp
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
#include "Basic.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NvGLUtils/NvShapesGL.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"

Basic::Basic()
{
    m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -2.2f));
    m_transformer->setRotationVec(nv::vec3f(NV_PI*0.35f, 0.0f, 0.0f));

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

Basic::~Basic()
{
    LOGI("Basic: destroyed\n");
}

void Basic::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionGL4();
}

void Basic::initRendering(void) {
    NvAssetLoaderAddSearchPath("tutorial/Basic");

    mProgram = NvGLSLProgram::createFromFiles("shaders/plain.vert", "shaders/plain.frag");

}

void Basic::shutdownRendering(void) {

    // destroy other resources here
}

void Basic::initUI(void) {
    if (mTweakBar) {
//        NvTweakVarBase *var;

        mTweakBar->syncValues();
    }
}


void Basic::reshape(int32_t width, int32_t height)
{
    glViewport( 0, 0, (GLint) width, (GLint) height );
}


void Basic::draw(void)
{
    CHECK_GL_ERROR();
    glClear(GL_COLOR_BUFFER_BIT);
    CHECK_GL_ERROR();

    nv::matrix4f projection_matrix;
    nv::perspective(projection_matrix, 3.14f * 0.25f, m_width/(float)m_height, 0.1f, 30.0f);
    nv::matrix4f camera_matrix = projection_matrix * m_transformer->getModelViewMat();
    CHECK_GL_ERROR();

    mProgram->enable();

    mProgram->setUniformMatrix4fv("uViewProjMatrix", camera_matrix._array, 1, GL_FALSE);
    CHECK_GL_ERROR();

    NvDrawQuadGL(mProgram->getAttribLocation("aPosition"));
    CHECK_GL_ERROR();

    mProgram->disable();
}

NvAppBase* NvAppFactory() {
    return new Basic();
}
