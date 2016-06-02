//----------------------------------------------------------------------------------
// File:        gl4-maxwell\NormalBlendedDecal/NormalBlendedDecal.cpp
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
#include "NormalBlendedDecal.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"
#include "NvGLUtils/NvImageGL.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NvGLUtils/NvModelGL.h"
#include "NvModel/NvModel.h"
#include "NvGLUtils/NvShapesGL.h"
#include "NvAppBase/NvInputTransformer.h"

#define SAFE_DELETE(p) { if( p ) {delete p; p = 0;} }

NormalBlendedDecal::NormalBlendedDecal()
    : mLock(LOCK_NONE)
    , mViewOption(VIEW_DEFAULT)
    , mBlitProgram(0)
    , mDecalProgram(0)
    , mGbufferProgram(0)
    , mCombinerProgram(0)
    , mNumDecals(20)
    , mSqrtNumModels(3)
    , mGbufferFBO(0)
    , mGbufferDecalFBO(0)
    , mModelDistance(2.f)
    , mDecalDistance(1.f)
    , mDecalSize(1.0f)
    , mBlendWeight(0.5f)
{
    for (int idx = 0; idx < NUM_GBUFFER_TEXTURES; idx++) {
        mGbufferTextures[idx] = 0;
    }
    m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -3.0f));
    m_transformer->setRotationVec(nv::vec3f(-0.0376263112f, -0.859024048f, 0.0f));
    m_transformer->setScale(0.232210547f);

    mDecalPos = new nv::vec2f[mNumDecals];

    for (uint32_t i = 0; i < mNumDecals; i++)
    {
        mDecalPos[i] = nv::vec2f(rand() / (float)RAND_MAX - 0.5f, 
            rand() / (float)RAND_MAX - 0.5f);
    }

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

NormalBlendedDecal::~NormalBlendedDecal()
{
    for (int idx = 0; idx < NUM_DECAL_BLENDS; ++idx) {
        glDeleteTextures(1, &mDecalNormals[idx]);
    }

    destroyShaders();
    destroyBuffers();
    LOGI("NormalBlendedDecal: destroyed\n");
}

NvModelGL* NormalBlendedDecal::loadModel(const char *model_filename)
{
    // Load model data for scene geometry
    int32_t length;
    char *modelData = NvAssetLoaderRead(model_filename, length);
	NvModelGL* model = NvModelGL::CreateFromObj(
		(uint8_t *)modelData, 1.0f, true, true);

    NvAssetLoaderFree(modelData);

    CHECK_GL_ERROR();
    return model;
}

void NormalBlendedDecal::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionGL4();
}

enum TweakBarActionCodes
{
    REACT_LOCK_METHOD = 1,
    REACT_SHOW_NORMALS = 2,
    REACT_SHOW_DECALS = 3,
};

void NormalBlendedDecal::initUI()
{
    // sample apps automatically have a tweakbar they can use.
    if (mTweakBar) { // create our tweak ui

        // Lock options
        bool isPSISupported = getGLContext()->isExtensionSupported("GL_NV_fragment_shader_interlock");
        if (isPSISupported) {
            NvTweakEnum<uint32_t> locks[] =
            {
                {"No Lock", LOCK_NONE},
                {"MemoryBarrier", LOCK_MEMORY_BARRIER},
                {"PixelShaderInterlock", LOCK_PIXEL_SHADER_INTERLOCK},
            };
            mTweakBar->addEnum("Lock Option:", (uint32_t&)mLock, locks, TWEAKENUM_ARRAYSIZE(locks), REACT_LOCK_METHOD);
        } else {
            NvTweakEnum<uint32_t> locks[] =
            {
                {"No Lock", LOCK_NONE},
                {"MemoryBarrier", LOCK_MEMORY_BARRIER},
            };
            mTweakBar->addEnum("Lock Option:", (uint32_t&)mLock, locks, TWEAKENUM_ARRAYSIZE(locks));
        }

        // View options
        NvTweakEnum<uint32_t> viewsOptions[] =
        {
            {"Default", VIEW_DEFAULT},
            {"Normals", VIEW_NORMALS},
            {"Decal Boxes", VIEW_DECAL_BOXES},
        };
        mTweakBar->addPadding();
        mTweakBar->addEnum("View Option:", (uint32_t&)mViewOption, viewsOptions, TWEAKENUM_ARRAYSIZE(viewsOptions));

        mTweakBar->addValue("Model Grid Size", mSqrtNumModels, 0, 10, 1);
#if (0) // Too many options
        mTweakBar->addValue("Model Distance", mModelDistance, 0.0f, 4.0f, 0.01f);
        mTweakBar->addValue("Number of Decals", mNumDecals, 0, 100, 1);
        mTweakBar->addValue("Decal Size", mDecalSize, 0.0f, 4.0f, 0.01f);
#endif

        // Decals options
        mTweakBar->addPadding();
        mTweakBar->addValue("Decal Distance", mDecalDistance, 0.0f, 4.0f, 0.01f);
        mTweakBar->addValue("Decal Blend Weight", mBlendWeight, 0.0f, 1.0f, 0.01f);

        // Model options
        mTweakBar->addPadding();
        NvTweakEnum<uint32_t> models[] =
        {
            {"Bunny", BUNNY_MODEL},
            {"Box", BOX_MODEL},
        };
        mTweakBar->addEnum("Models:", mModelID, models, TWEAKENUM_ARRAYSIZE(models));
    }
}

NvUIEventResponse NormalBlendedDecal::handleReaction(const NvUIReaction& react)
{
    switch(react.code)
    {
    case REACT_LOCK_METHOD:
        reloadShaders();
        return nvuiEventHandled;
    default:
        break;
    }
    return nvuiEventNotHandled;
}

void NormalBlendedDecal::initRendering(void)
{
    if (!requireMinAPIVersion(NvGLAPIVersionGL4_3()))
        return;

    //if (!requireExtension("GL_NV_fragment_shader_interlock"))
    //    return;

    bool isPSISupported = getGLContext()->isExtensionSupported("GL_NV_fragment_shader_interlock");
    if (isPSISupported) {
        mLock = LOCK_PIXEL_SHADER_INTERLOCK;
    } else {
        mLock = LOCK_MEMORY_BARRIER;
    }

    NvAssetLoaderAddSearchPath("gl4-maxwell/NormalBlendedDecal");

    if (!buildShaders()) {
        LOGI("Shader build error");
        return;
    }

    mCube = loadModel("models/cube.obj");
    mModels[BUNNY_MODEL] = loadModel("models/bunny2.obj");
    mModels[BOX_MODEL] = loadModel("models/cube.obj");
    mModelID = BUNNY_MODEL;

    mDecalNormals[0] = NvImageGL::UploadTextureFromDDSFile("textures/rock_normal.dds");
    mDecalNormals[1] = NvImageGL::UploadTextureFromDDSFile("textures/brick_normal.dds");

    // Disable wait for vsync
    getGLContext()->setSwapInterval(0);
}

void NormalBlendedDecal::destroyBuffers()
{
    glDeleteFramebuffers(1, &mGbufferFBO);
    glDeleteFramebuffers(1, &mGbufferDecalFBO);
    for (int idx = 0; idx < NUM_GBUFFER_TEXTURES; ++idx) {
        glDeleteTextures(NUM_GBUFFER_TEXTURES, mGbufferTextures);
    }
}

void NormalBlendedDecal::reshape(int32_t width, int32_t height)
{
    glViewport(0, 0, (GLint) width, (GLint) height);

    destroyBuffers();

    // Build buffers
    if ((width > 0) && (height > 0)) {
        glGenFramebuffers(1, &mGbufferFBO);

        glBindFramebuffer(GL_FRAMEBUFFER, mGbufferFBO);
        glGenTextures(NUM_GBUFFER_TEXTURES, mGbufferTextures);
        
        // uColorTex
        glBindTexture(GL_TEXTURE_2D, mGbufferTextures[COLOR_TEXTURE]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, m_width, m_height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // uNormalTex
        glBindTexture(GL_TEXTURE_2D, mGbufferTextures[NORMAL_TEXTURE]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, m_width, m_height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // uWorldPosTex
        glBindTexture(GL_TEXTURE_2D, mGbufferTextures[WORLDPOS_TEXTURE]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Depth texture
        glBindTexture(GL_TEXTURE_2D, mGbufferTextures[DEPTH_TEXTURE]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mGbufferTextures[COLOR_TEXTURE], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mGbufferTextures[NORMAL_TEXTURE], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, mGbufferTextures[WORLDPOS_TEXTURE], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mGbufferTextures[DEPTH_TEXTURE], 0);

        glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());

        // Set GbufferDecalFBO
        glGenFramebuffers(1, &mGbufferDecalFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, mGbufferDecalFBO);
        
        // Only color output
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mGbufferTextures[COLOR_TEXTURE], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mGbufferTextures[DEPTH_TEXTURE], 0);

        glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
    }
}

void NormalBlendedDecal::draw2d( GLuint texture, NvGLSLProgram* program )
{
    glViewport( 0 , 0 , m_width , m_height );
    
    glUseProgram(program->getProgram());

    if (texture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(program->getUniformLocation("uSourceTex"), 0);
    }
    
    program->setUniformMatrix4fv("uViewMatrix", m_transformer->getModelViewMat()._array);
    int aPosCoord = program->getAttribLocation("aPosition");
    int aTexCoord = program->getAttribLocation("aTexCoord");
    NvDrawQuadGL(aPosCoord, aTexCoord);
}

void NormalBlendedDecal::draw(void)
{
    if ((m_width == 0) || (m_height == 0)) return;

    nv::matrix4f proj;
    nv::perspective(proj, 45.0f, (GLfloat)m_width / (GLfloat)m_height, 0.01f, 100.0f);

    float uScreenSize[] = { (float)m_width, (float)m_height, 0, 0 };
    static const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // Draw world geometries into gbuffer
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mGbufferFBO);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Enable all color buffers
        glDrawBuffers(3, drawBuffers);

        // Clear render targets
        static const GLfloat floatZeroes[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        static const GLfloat floatOnes[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glClearBufferfv(GL_COLOR, 0, floatZeroes);
        glClearBufferfv(GL_COLOR, 1, floatZeroes);
        glClearBufferfv(GL_COLOR, 2, floatZeroes);
        glClearBufferfv(GL_DEPTH, 0, floatOnes);
        
        // Enable GBuffer program
        mGbufferProgram->enable();

        // Draw scene objects
        for (unsigned int j = 0; j < mSqrtNumModels; j++) {
            for (unsigned int i = 0; i < mSqrtNumModels; i++) {
                // Model transform
                nv::matrix4f matModel;
                matModel.set_scale(1.0f);
                matModel.set_translate(nv::vec3f(
                    .5f*mModelDistance*(mSqrtNumModels - 1.0f) - mModelDistance*i, 
                    .5f*mModelDistance*(mSqrtNumModels - 1.0f) - mModelDistance*j, 0.0f));

                // Model view transform
                nv::matrix4f matMV = m_transformer->getModelViewMat() * matModel;

                // Movel view projection transform
                nv::matrix4f matMVP = proj * matMV;

                // Set uniforms
                mGbufferProgram->setUniformMatrix4fv("uModelViewMatrix", matMV._array);
                mGbufferProgram->setUniformMatrix4fv("uModelViewProjMatrix", matMVP._array);
                mGbufferProgram->setUniformMatrix4fv("uModelMatrix", matModel._array);

                // Draw model
                mModels[mModelID]->drawElements(0, 1, 2, 3);
            }
        }
    }

    // Draw decal box into gbuffer
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mGbufferDecalFBO);
        
        // Enable only color buffer
        glDrawBuffers(1, drawBuffers);
      
        if (mViewOption == VIEW_DECAL_BOXES) {
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBlendFunc(GL_ONE, GL_ONE);
        }

        mDecalProgram->enable();

        // uNormalImage
        glBindImageTexture(0, mGbufferTextures[NORMAL_TEXTURE], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

        // uWorldPosTex
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mGbufferTextures[WORLDPOS_TEXTURE]);

        mDecalProgram->setUniform4fv("uScreenSize", uScreenSize);
        mDecalProgram->setUniform1i("uShowDecalBox", (mViewOption == VIEW_DECAL_BOXES) ? true : false);

        float gridSize = mModelDistance*(mSqrtNumModels - 1.0f);

        for (unsigned int idx = 0; idx < mNumDecals; idx++) {
            nv::matrix4f matDecal;
            matDecal.make_identity();
            matDecal.set_scale(nv::vec3f(mDecalSize, mDecalSize, 2.0f));
            matDecal.set_translate(nv::vec3f(gridSize*mDecalDistance*mDecalPos[idx].x,
                gridSize*mDecalDistance*mDecalPos[idx].y, 0));
            nv::matrix4f matMVP = proj * m_transformer->getModelViewMat() * matDecal;
            nv::matrix4f invMVP = inverse(matMVP);
            nv::matrix4f invModel = inverse(matDecal);
            mDecalProgram->setUniformMatrix4fv("uModelMatrix", matDecal._array);
            mDecalProgram->setUniformMatrix4fv("uInvModelMatrix", invModel._array);
            mDecalProgram->setUniformMatrix4fv("uModelViewProjMatrix", matMVP._array);
            mDecalProgram->setUniformMatrix4fv("uViewMatrix", m_transformer->getModelViewMat()._array);

            float weight = mBlendWeight;
            for (int blendIdx = 0; blendIdx < NUM_DECAL_BLENDS; ++blendIdx) {
                // uDecalNormalTex
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, mDecalNormals[blendIdx]);

                mDecalProgram->setUniform1f("uBlendWeight", weight);
                weight = (blendIdx == (NUM_DECAL_BLENDS - 2)) ? (1.0f - weight) : weight*mBlendWeight;

                // Draw decal cube
                mCube->drawElements(0, 1, 2, 3);

                if (mLock == LOCK_MEMORY_BARRIER) {
                    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                }
            }
        }

        // Restore states
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

    // Draw combiner pass
    {
        // Back to main FBO
        glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());

        // Disable depth testing
        glDisable(GL_DEPTH_TEST);

        // Enable combiner program
        mCombinerProgram->enable();

        // uColorTex
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mGbufferTextures[COLOR_TEXTURE]);

        // uNormalTex
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mGbufferTextures[NORMAL_TEXTURE]);

        // Set decal box option
        mCombinerProgram->setUniform1i("uShowDecalBox", (mViewOption == VIEW_DECAL_BOXES) ? true : false);

        // Set decal box option
        mCombinerProgram->setUniform1i("uShowNormals", (mViewOption == VIEW_NORMALS) ? true : false);

        // Set light direction
        nv::matrix4f invViewMatrix = inverse(m_transformer->getModelViewMat());
        nv::vec4f lightDir = invViewMatrix * nv::vec4f(0, 0, 1.0f, 0);
        lightDir = normalize(lightDir);
        mCombinerProgram->setUniform4fv("uLightDir", lightDir);

        // Set view direction
        nv::vec4f viewDir = invViewMatrix * nv::vec4f(0, 0, -1.0f, 0);
        viewDir = normalize(viewDir);
        mCombinerProgram->setUniform4fv("uViewDir", viewDir);

        // Draw combiner program with screen quad
        draw2d(0, mCombinerProgram);
        
        // Restore states
        glEnable(GL_DEPTH_TEST);
    }
}

NvAppBase* NvAppFactory()
{
    return new NormalBlendedDecal();
}

bool NormalBlendedDecal::buildShaders()
{
    if (mLock == LOCK_PIXEL_SHADER_INTERLOCK) {
        NvGLSLProgram::setGlobalShaderHeader("#version 430\n#define USE_PIXEL_SHADER_INTERLOCK 1");
    } else {
        NvGLSLProgram::setGlobalShaderHeader("#version 430\n");
    }

    printf("\nloading shaders...\n");
    
    mDecalProgram = NvGLSLProgram::createFromFiles("shaders/base_vertex.glsl",
        "shaders/decal_fragment.glsl");
    if (!mDecalProgram) return false;

    mBlitProgram= NvGLSLProgram::createFromFiles("shaders/plain_vertex.glsl",
        "shaders/plain_fragment.glsl");
    if (!mBlitProgram) return false;

    mGbufferProgram = NvGLSLProgram::createFromFiles("shaders/base_vertex.glsl",
        "shaders/gbuffer_fragment.glsl");
    if (!mGbufferProgram) return false;

    mCombinerProgram = NvGLSLProgram::createFromFiles("shaders/plain_vertex.glsl",
        "shaders/combiner_fragment.glsl");
    if (!mCombinerProgram) return false;

    NvGLSLProgram::setGlobalShaderHeader(NULL);

    return true;
}

void NormalBlendedDecal::destroyShaders()
{
    SAFE_DELETE(mBlitProgram);
    SAFE_DELETE(mDecalProgram);
    SAFE_DELETE(mGbufferProgram);
    SAFE_DELETE(mCombinerProgram);
}

void NormalBlendedDecal::reloadShaders()
{
    destroyShaders();
    buildShaders();
}
