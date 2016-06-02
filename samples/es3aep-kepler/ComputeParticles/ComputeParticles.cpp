//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ComputeParticles/ComputeParticles.cpp
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
#include "ComputeParticles.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"
#include <string>

#include "ParticleSystem.h"

extern std::string loadShaderSourceWithUniformTag(const char* uniformsFile, const char* srcFile);

enum UIReactionIDs {   
    REACT_RESET         = 0x10000001,
};

ComputeParticles::ComputeParticles() 
    : mEnableAttractor(false),
    mAnimate(true),
    mTime(0.0f)
{
    m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -3.0f));

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

ComputeParticles::~ComputeParticles()
{
    LOGI("ComputeParticles: destroyed\n");
}

void ComputeParticles::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionES3_1();
}

void ComputeParticles::initRendering(void) { 
    // OpenGL 4.3 is the minimum for compute shaders
    if (!requireMinAPIVersion(NvGLAPIVersionES3_1()))
        return;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);    

    CHECK_GL_ERROR();
    NvAssetLoaderAddSearchPath("es3aep-kepler/ComputeParticles"); 

    const char* shaderPrefix =
        (getGLContext()->getConfiguration().apiVer.api == NvGLAPI::GL)
        ? "#version 430\n" : "#version 310 es\n";

    CHECK_GL_ERROR();
    {
        NvScopedShaderPrefix switched(shaderPrefix);

        std::string renderVS = loadShaderSourceWithUniformTag("shaders/uniforms.h", "shaders/renderVS.glsl");
        mRenderProg = new NvGLSLProgram;
    
        int32_t len;
        NvGLSLProgram::ShaderSourceItem sources[2];
        sources[0].type = GL_VERTEX_SHADER;
        sources[0].src = renderVS.c_str();
        sources[1].type = GL_FRAGMENT_SHADER;
        sources[1].src = NvAssetLoaderRead("shaders/renderFS.glsl", len);
        mRenderProg->setSourceFromStrings(sources, 2);
        NvAssetLoaderFree((char*)sources[1].src);
    }
    CHECK_GL_ERROR();

    //create ubo and initialize it with the structure data
    glGenBuffers( 1, &mUBO);
    glBindBuffer( GL_UNIFORM_BUFFER, mUBO);
    glBufferData( GL_UNIFORM_BUFFER, sizeof(ShaderParams), &mShaderParams, GL_STREAM_DRAW);
    CHECK_GL_ERROR();

    //create simple single-vertex VBO
    float vtx_data[] = { 0.0f, 0.0f, 0.0f, 1.0f};
    glGenBuffers( 1, &mVBO);
    glBindBuffer( GL_ARRAY_BUFFER, mVBO);
    glBufferData( GL_ARRAY_BUFFER, sizeof(vtx_data), vtx_data, GL_STATIC_DRAW);
    CHECK_GL_ERROR();

    // For now, scale back the particle count on mobile.
    mParticleCount = isMobilePlatform() ? (mNumParticles >> 2) : mNumParticles;
    mParticles = new ParticleSystem(mParticleCount, shaderPrefix);
    CHECK_GL_ERROR();

    int cx, cy, cz;
    glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &cx );
    glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &cy );
    glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &cz );
    LOGI("Max compute work group count = %d, %d, %d\n", cx, cy, cz );

    int sx, sy, sz;
    glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &sx );
    glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &sy );
    glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &sz );
    LOGI("Max compute work group size  = %d, %d, %d\n", sx, sy, sz );

    CHECK_GL_ERROR();
}

void ComputeParticles::initUI(void) {
    if (mTweakBar) {
        NvTweakVarBase *var;

        mTweakBar->addPadding();
        var = mTweakBar->addValue("Animate", mAnimate);
        addTweakKeyBind(var, NvKey::K_SPACE);
        addTweakButtonBind(var, NvGamepad::BUTTON_Y);
        var = mTweakBar->addValue("Enable attractor", mEnableAttractor);
        addTweakKeyBind(var, NvKey::K_A);
        addTweakButtonBind(var, NvGamepad::BUTTON_X);
        
        mTweakBar->addPadding();
        var = mTweakBar->addValue("Sprite size", mShaderParams.spriteSize, 0.0f, 0.025f, 0.005f);
        addTweakKeyBind(var, NvKey::K_RBRACKET, NvKey::K_LBRACKET);
        addTweakButtonBind(var, NvGamepad::BUTTON_RIGHT_SHOULDER, NvGamepad::BUTTON_LEFT_SHOULDER);
        var = mTweakBar->addValue("Noise strength", mShaderParams.noiseStrength, 0.0f, 0.01f, 0.001f);
        var = mTweakBar->addValue("Noise frequency", mShaderParams.noiseFreq, 0.0f, 20.0f);
        
        mTweakBar->addPadding();
        var = mTweakBar->addButton("Reset", REACT_RESET);
        addTweakKeyBind(var, NvKey::K_R);
        addTweakButtonBind(var, NvGamepad::BUTTON_RIGHT_THUMB);
    }
}

NvUIEventResponse ComputeParticles::handleReaction(const NvUIReaction &react)
{
    switch (react.code) {
        case REACT_RESET:
            mTweakBar->resetValues();
            // have to manually reset some things...
            mTime = 0.0f; 
            mParticles->reset(0.5f); // the same as ParticleSystem constructor uses...
            return nvuiEventHandled;
        }
    return nvuiEventNotHandled;
}

void ComputeParticles::reshape(int32_t width, int32_t height)
{
    glViewport( 0, 0, (GLint) width, (GLint) height );

    CHECK_GL_ERROR();
}

void ComputeParticles::draw(void)
{
    glClearColor( 0.25f, 0.25f, 0.25f, 1.0f);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //
    // Compute matrices without the legacy matrix stack support
    //
    nv::matrix4f projectionMatrix;
    nv::perspective( projectionMatrix, 45.0f * 2.0f*3.14159f / 360.0f, (float)m_width/(float)m_height, 0.1f, 10.0f);

    nv::matrix4f viewMatrix = m_transformer->getModelViewMat();

    //
    // update struct representing UBO
    //
    mShaderParams.numParticles = (unsigned int) (mParticles->getSize());
    mShaderParams.ModelView = viewMatrix;
    mShaderParams.ModelViewProjection = projectionMatrix * viewMatrix;
    mShaderParams.ProjectionMatrix = projectionMatrix;

    if (mEnableAttractor) {
        // move attractor
        const float speed = 0.2f;
        mShaderParams.attractor.x = sin(mTime*speed);
        mShaderParams.attractor.y = sin(mTime*speed*1.3f);
        mShaderParams.attractor.z = cos(mTime*speed);
        mTime += getFrameDeltaTime();

        mShaderParams.attractor.w = 0.0002f;
    } else {
        mShaderParams.attractor.w = 0.0f;
    }

    glActiveTexture(GL_TEXTURE0);

    // bind the buffer for the UBO, and update it with the latest values from the CPU-side struct
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, mUBO);
    glBindBuffer( GL_UNIFORM_BUFFER, mUBO);
    glBufferSubData( GL_UNIFORM_BUFFER, 0, sizeof(ShaderParams), &mShaderParams);

    if (mAnimate) {
        mParticles->update();
    } 

    // draw particles
    mRenderProg->enable();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive blend

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // reference the compute shader buffer, which we will use for the particle
    // locations (only the positions for now)
    glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  mParticles->getPosBuffer()->getBuffer() );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mParticles->getIndexBuffer()->getBuffer() );

    glDrawElements(GL_TRIANGLES, GLsizei(mParticles->getSize()*6), GL_UNSIGNED_INT, 0);

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  0 );

    glDisable(GL_BLEND);

    mRenderProg->disable();
}


// A replacement for the ARB_shader_include extension, which is not widely supported
// This loads a "header" file (uniformsFile) and a "source" file (srcFile).  It scans
// the source file for "#UNIFORMS" and replaces this tag with the contents of the
// uniforms file.  Limited, but functional for the cases used here
std::string loadShaderSourceWithUniformTag(const char* uniformsFile, const char* srcFile) {
    int32_t len;

    char *uniformsStr = NvAssetLoaderRead(uniformsFile, len);
    if (!uniformsStr)
        return "";

    char *srcStr = NvAssetLoaderRead(srcFile, len);
    if (!srcStr)
        return "";

    std::string dest = "";

    const char* uniformTag = "#UNIFORMS";

    char* uniformTagStart = strstr(srcStr, uniformTag);

    if (uniformTagStart) {
        // NULL the start of the tag
        *uniformTagStart = 0;
        dest += srcStr; // source up to tag
        dest += "\n";
        dest += uniformsStr;
        dest += "\n";
        char* uniformTagEnd = uniformTagStart + strlen(uniformTag);
        dest += uniformTagEnd;
    } else {
        dest += srcStr;
    }

    NvAssetLoaderFree(uniformsStr);
    NvAssetLoaderFree(srcStr);

    return dest;
}

NvAppBase* NvAppFactory() {
    return new ComputeParticles();
}
