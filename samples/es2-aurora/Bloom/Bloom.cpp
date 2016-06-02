//----------------------------------------------------------------------------------
// File:        es2-aurora\Bloom/Bloom.cpp
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
#include "Bloom.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NvGLUtils/NvImageGL.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"
#ifndef _WIN32
#include <unistd.h>
#endif

#include "AppExtensions.h"

// rendering defs /////////////////////////////////////////////////////////////////////////
#define Z_NEAR 0.4f
#define Z_FAR 500.0f
#define FOV 3.14f*0.5f

#define SHADOWMAP_SIZE 1024

uint32_t gLumaTypeEnum;


Bloom::Bloom() 
{
    m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -2.2f));
    m_transformer->setRotationVec(nv::vec3f(NV_PI*0.35f, 0.0f, 0.0f));

    mLightPosition = nv::vec3f(1000.0f, 1000.0f, 0.0f);

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();

    mAnimationTime = 0.0f;
    mDrawBloom = true;
    mDrawGlow = true;
    mDrawShadow = true;
    mBlurWidth = 4.0f;
    mBloomIntensity = 3.0f;
    mAutoSpin = true;
}

Bloom::~Bloom()
{
    LOGI("Bloom: destroyed\n");
}

void Bloom::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    // We'd prefer ES3 (where shadows are in core), but we'll take ES2 later if it has extensions
    config.apiVer = NvGLAPIVersionES3();
}

void Bloom::initRendering(void) {
    if (getGLContext()->getConfiguration().apiVer == NvGLAPIVersionES2()) {
        gLumaTypeEnum = GL_LUMINANCE;
    } else {
        gLumaTypeEnum = 0x1903; // GL_RED, not declared in ES
    }

    //glClearColor(0.5f, 0.5f, 0.5f, 1.0f); 
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);    

    NvAssetLoaderAddSearchPath("es2-aurora/Bloom");

    mQuadProgram = NvGLSLProgram::createFromFiles("shaders/quad.vp", "shaders/quad.fp");
    mBlurProgram = NvGLSLProgram::createFromFiles("shaders/fullscreen_quad.vp", "shaders/blur.fp");
    mCombineProgram = NvGLSLProgram::createFromFiles("shaders/fullscreen_quad.vp", "shaders/combine.fp");
    mDownfilterProgram = NvGLSLProgram::createFromFiles("shaders/fullscreen_quad.vp", "shaders/downfilter.fp");

    const NvGLAPIVersion& api = getGLContext()->getConfiguration().apiVer;

	if (api.api == NvGLAPI::GL) {
		mGateProgram = NvGLSLProgram::createFromFiles("shaders/gate_gl4.vp", "shaders/gate_gl4.fp");
	} else if ((api.api == NvGLAPI::GLES) && (api.majVersion == 3)) {
		mGateProgram = NvGLSLProgram::createFromFiles("shaders/gate_es3.vp", "shaders/gate_es3.fp");
    } else {
        mGateProgram = NvGLSLProgram::createFromFiles("shaders/gate.vp", "shaders/gate.fp");
    }
    mGateShadowProgram = NvGLSLProgram::createFromFiles("shaders/gate_shadow.vp", "shaders/gate_shadow.fp");

    createOSDVBOs();
    createGateVBOs();

    if (requireExtension("GL_EXT_texture_compression_dxt1", false)) {
        mGateDiffuseTex = NvImageGL::UploadTextureFromDDSFile("textures/gate_diffuse_dxt1.dds");
        mGateGlowTex = NvImageGL::UploadTextureFromDDSFile("textures/gate_glow_dxt1.dds");
    } else {
        mGateDiffuseTex = NvImageGL::UploadTextureFromDDSFile("textures/gate_diffuse_rgb.dds");
        mGateGlowTex = NvImageGL::UploadTextureFromDDSFile("textures/gate_glow_rgb.dds");
    }
    glBindTexture(GL_TEXTURE_2D, mGateDiffuseTex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, mGateGlowTex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);

    CHECK_GL_ERROR();
}

void Bloom::initUI(void) {
    if (mTweakBar) {
        NvTweakVarBase *var;

        var = mTweakBar->addValue("Auto Spin", mAutoSpin);
        var = mTweakBar->addValue("Draw Bloom", mDrawBloom);
        var = mTweakBar->addValue("Bloom Intensity", mBloomIntensity, 1.0f, 10.0f);
        mTweakBar->addPadding();
        var = mTweakBar->addValue("Draw Glow", mDrawGlow);
        var = mTweakBar->addValue("Draw Shadow", mDrawShadow);
        mTweakBar->addPadding();
        var = mTweakBar->addValue("Blur width", mBlurWidth, 2.0f, 20.0f);

        mTweakBar->syncValues();
    }
}


void Bloom::reshape(int32_t width, int32_t height)
{
    glViewport( 0, 0, (GLint) width, (GLint) height );

    int w=width/4,h=height/4;

    initFBO(width,height,GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 0, GL_DEPTH_COMPONENT16, mFBOs.scene);
    initFBO(w,h,GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 0, 0, mFBOs.halfres);
    initFBO(w,h,GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 0, 0, mFBOs.blurred);

    initDepthTexFBO(SHADOWMAP_SIZE,SHADOWMAP_SIZE,gLumaTypeEnum, gLumaTypeEnum, GL_UNSIGNED_BYTE,
        0, mFBOs.shadowmap);

    CHECK_GL_ERROR();
}

void Bloom::draw(void)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_transformer->setRotationVel(nv::vec3f(0.0f, mAutoSpin ? (NV_PI*0.05f) : 0.0f, 0.0f));

    updateDynamics();

    renderScene();
    
    mAnimationTime += getFrameDeltaTime();
}

void Bloom::renderShadowGate()
{
    mGateShadowProgram->enable();
    
    mGateShadowProgram->setUniformMatrix4fv("u_ViewProjMatrix", 
        mDynamics.mShadowMVP._array, 1, GL_FALSE);

    mGateShadowProgram->setUniform3f("u_LightVector", mDynamics.mWorldLightDir.x, 
        mDynamics.mWorldLightDir.y, mDynamics.mWorldLightDir.z);

    glBindBuffer(GL_ARRAY_BUFFER, mGateModel.vertex_vb);

    GLint posIndex = mGateShadowProgram->getAttribLocation("a_Position");
    glEnableVertexAttribArray(posIndex);
    CHECK_GL_ERROR();

    glVertexAttribPointer(mGateShadowProgram->getAttribLocation("a_Position"), 3, GL_FLOAT, GL_FALSE, 9*sizeof(float), 0);
    CHECK_GL_ERROR();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mGateModel.vertex_ib);

    glDrawElements(GL_TRIANGLES, mGateModel.numindices, GL_UNSIGNED_SHORT,0);

    glDisableVertexAttribArray(posIndex);
}


void Bloom::renderGate()
{
    mGateProgram->enable();

    nv::matrix4f light_scale_matrix;
    light_scale_matrix.make_identity();
    light_scale_matrix.set_scale(-0.5f);
    light_scale_matrix.set_translate(nv::vec3f(0.5f, 0.5f, 0.5f));

    nv::matrix4f light_final_matrix = light_scale_matrix * mDynamics.mShadowMVP;
    mGateProgram->setUniformMatrix4fv("u_LightMatrix", light_final_matrix._array, 1, GL_FALSE);

    mGateProgram->setUniformMatrix4fv("u_ViewProjMatrix", mDynamics.mCameraMVP._array, 1, GL_FALSE);

    mGateProgram->setUniform3f("u_LightVector", mDynamics.mWorldLightDir.x, 
        mDynamics.mWorldLightDir.y, mDynamics.mWorldLightDir.z);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mGateDiffuseTex);
    mGateProgram->setUniform1i("diffuse_tex", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mFBOs.shadowmap.depth_texture);
    mGateProgram->setUniform1i("variance_shadowmap_tex", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mGateGlowTex);
    mGateProgram->setUniform1i("glow_tex", 2);

    float intensity = mDrawGlow ? (float)sin(mAnimationTime) : 0.0f;
    intensity *= intensity;

    mGateProgram->setUniform1f("u_glowIntensity", intensity);

    glBindBuffer(GL_ARRAY_BUFFER, mGateModel.vertex_vb);

    GLint posIndex = mGateProgram->getAttribLocation("a_Position");
    glEnableVertexAttribArray(posIndex);
    CHECK_GL_ERROR();
    glVertexAttribPointer(posIndex, 3, GL_FLOAT, GL_FALSE, 9*sizeof(float), 0);
    CHECK_GL_ERROR();

    GLint normalIndex = mGateProgram->getAttribLocation("a_Normal");
    if (normalIndex >= 0) {
        glEnableVertexAttribArray(normalIndex);
        CHECK_GL_ERROR();
        glVertexAttribPointer(normalIndex, 3, GL_FLOAT, GL_FALSE, 9*sizeof(float), (const GLvoid*)(3*sizeof(float)));
        CHECK_GL_ERROR();
    }

    GLint uvIndex = mGateProgram->getAttribLocation("a_TexCoord");
    if (uvIndex >= 0) {
        glEnableVertexAttribArray(uvIndex);
        CHECK_GL_ERROR();
        glVertexAttribPointer(uvIndex, 3, GL_FLOAT, GL_FALSE, 9*sizeof(float), (const GLvoid*)(6*sizeof(float)));
        CHECK_GL_ERROR();
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mGateModel.vertex_ib);

    glDrawElements(GL_TRIANGLES, mGateModel.numindices, GL_UNSIGNED_SHORT,0);

    glDisableVertexAttribArray(posIndex);

    if (normalIndex >= 0)
        glDisableVertexAttribArray(normalIndex);

    if (uvIndex >= 0)
        glDisableVertexAttribArray(uvIndex);
}



void Bloom::renderScene() {
    nv::vec2f quad_scale;
    nv::vec2f quad_position;

    CHECK_GL_ERROR();
    
    // rendering shadow
    //selecting shadow FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mFBOs.shadowmap.fbo);
    glViewport(0,0,SHADOWMAP_SIZE,SHADOWMAP_SIZE);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    CHECK_GL_ERROR();

    if (mDrawShadow)
        renderShadowGate();

    // selecting scene FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mFBOs.scene.fbo);
    glViewport(0,0,m_width,m_height);

    CHECK_GL_ERROR();

    // selecting scene FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mDrawBloom ? mFBOs.scene.fbo : getMainFBO());
    glViewport(0,0,m_width,m_height);
    glClearColor(0.1f, 0.1f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    CHECK_GL_ERROR();

    renderGate();

    CHECK_GL_ERROR();
    
    if (mDrawBloom) {
        // selecting downfilter FBO
        glBindFramebuffer(GL_FRAMEBUFFER, mFBOs.halfres.fbo);
        glViewport(0,0,m_width/4,m_height/4);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        // rendering scene buffer to downsample FBO
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,mFBOs.scene.color_texture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        mDownfilterProgram->enable();
        mDownfilterProgram->setUniform1i("texture_to_downfilter", 0);
        CHECK_GL_ERROR();
        mDownfilterProgram->setUniform2f("u_TexelOffsets", 
            1.0f/(float)m_width, 1.0f/(float)m_height);

        glBindBuffer(GL_ARRAY_BUFFER, mQuadVBO);
        glEnableVertexAttribArray(mDownfilterProgram->getAttribLocation("a_Position"));
        glEnableVertexAttribArray(mDownfilterProgram->getAttribLocation("a_TexCoord"));
        glVertexAttribPointer(mDownfilterProgram->getAttribLocation("a_Position"), 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), 0);
        glVertexAttribPointer(mDownfilterProgram->getAttribLocation("a_TexCoord"), 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (const GLvoid*)(2*sizeof(float)));
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        
        // selecting blur FBO
        glBindFramebuffer(GL_FRAMEBUFFER, mFBOs.blurred.fbo);
        glViewport(0,0,m_width/4,m_height/4);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        // rendering scene buffer to blur FBO
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,mFBOs.halfres.color_texture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        mBlurProgram->enable();
        mBlurProgram->setUniform1i("texture_to_blur", 0);
        mBlurProgram->setUniform2f("u_TexelOffsets", mBlurWidth/(float)m_width, 0.0f);

        glBindBuffer(GL_ARRAY_BUFFER, mQuadVBO);
        glEnableVertexAttribArray(mBlurProgram->getAttribLocation("a_Position"));
        glEnableVertexAttribArray(mBlurProgram->getAttribLocation("a_TexCoord"));
        glVertexAttribPointer(mBlurProgram->getAttribLocation("a_Position"), 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), 0);
        glVertexAttribPointer(mBlurProgram->getAttribLocation("a_TexCoord"), 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (const GLvoid*)(2*sizeof(float)));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CHECK_GL_ERROR();

        // selecting downfilter FBO (will be blurred_pong)
        glBindFramebuffer(GL_FRAMEBUFFER, mFBOs.halfres.fbo);
        glViewport(0,0,m_width/4,m_height/4);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        // rendering scene buffer to downfilter FBO
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,mFBOs.blurred.color_texture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

        mBlurProgram->enable();
        mBlurProgram->setUniform1i("texture_to_blur", 0);
        mBlurProgram->setUniform2f("u_TexelOffsets", 0.0f, mBlurWidth/(float)m_height);
        glBindBuffer(GL_ARRAY_BUFFER, mQuadVBO);
        glVertexAttribPointer(mBlurProgram->getAttribLocation("a_Position"), 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), 0);
        glVertexAttribPointer(mBlurProgram->getAttribLocation("a_TexCoord"), 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (const GLvoid*)(2*sizeof(float)));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CHECK_GL_ERROR();

        // selecting backbuffer
        glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
        glViewport(0,0,m_width,m_height);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        CHECK_GL_ERROR();

        // combining buffers to backbuffer
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,mFBOs.halfres.color_texture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        CHECK_GL_ERROR();

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D,mFBOs.scene.color_texture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        CHECK_GL_ERROR();

        mCombineProgram->enable();
        mCombineProgram->setUniform1i("blurred_texture", 0);
        mCombineProgram->setUniform1i("original_texture", 1);
        mCombineProgram->setUniform1f("u_glowIntensity", mBloomIntensity);
        glBindBuffer(GL_ARRAY_BUFFER, mQuadVBO);
        glEnableVertexAttribArray(mCombineProgram->getAttribLocation("a_Position"));
        glEnableVertexAttribArray(mCombineProgram->getAttribLocation("a_TexCoord"));
        glVertexAttribPointer(mCombineProgram->getAttribLocation("a_Position"), 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), 0);
        glVertexAttribPointer(mCombineProgram->getAttribLocation("a_TexCoord"), 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (const GLvoid*)(2*sizeof(float)));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CHECK_GL_ERROR();
    
        glBlendFunc(GL_ONE,GL_ONE);
        glDisable(GL_BLEND);

        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        CHECK_GL_ERROR();
    }

#if 0
    int vert = 0;
    const int thumbHeight = 256;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mFBOs.shadowmap.fbo);        CHECK_GL_ERROR();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, getMainFBO());        CHECK_GL_ERROR();

    glBlitFramebuffer(0, 0, mFBOs.shadowmap.width, mFBOs.shadowmap.height,
                        0, vert, (thumbHeight*mFBOs.shadowmap.width)/mFBOs.shadowmap.height, vert+thumbHeight,
                        GL_COLOR_BUFFER_BIT, GL_NEAREST);        CHECK_GL_ERROR();
    vert += thumbHeight + 20;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, mFBOs.scene.fbo);        CHECK_GL_ERROR();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, getMainFBO());        CHECK_GL_ERROR();

    glBlitFramebuffer(0, 0, mFBOs.scene.width, mFBOs.scene.height,
                        0, vert, (thumbHeight*mFBOs.scene.width)/mFBOs.scene.height, vert+thumbHeight,
                        GL_COLOR_BUFFER_BIT, GL_NEAREST);        CHECK_GL_ERROR();
    vert += thumbHeight + 20;
#endif
}


bool Bloom::createOSDVBOs ()
{
    float data[16]=    {0.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f, 1.0f,
                        1.0f, 0.0f, 1.0f, 0.0f,
                        1.0f, 1.0f, 1.0f, 1.0f};

    glGenBuffers(1,&mQuadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, 16*sizeof(float),data, GL_STATIC_DRAW) ;
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}


bool Bloom::createGateVBOs ()
{

    typedef struct
    {
        nv::vec3f position;
        nv::vec3f normal;
        nv::vec3f texcoord;

    } TempVertType;

    TempVertType *tempVerts;
    GLushort *tempIndices;

    const uint32_t numvertices=14400;
    const uint32_t numindices=21600;

    // loading data from asset
    int32_t len;
    char* gateData = NvAssetLoaderRead("raw/gate.dat", len);
    if(gateData==NULL)
    {
        LOGI(" - could not open _asset [%s]","gate.dat");
        return false;
    }
    else
    {
        LOGI( " - asset [%s] opened successfully","gate.dat");
    }

    mGateModel.numvertices=numvertices;
    mGateModel.numindices=numindices;
    tempVerts = (TempVertType *)gateData;
    tempIndices = (GLushort *)(gateData + mGateModel.numvertices*9*sizeof(float));

    glGenBuffers(1,&mGateModel.vertex_vb);
    glBindBuffer(GL_ARRAY_BUFFER, mGateModel.vertex_vb);
    glBufferData(GL_ARRAY_BUFFER, mGateModel.numvertices*9*sizeof(float),tempVerts, GL_STATIC_DRAW) ;
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    glGenBuffers(1,&mGateModel.vertex_ib);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mGateModel.vertex_ib);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mGateModel.numindices*sizeof(GLushort),tempIndices, GL_STATIC_DRAW) ;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    CHECK_GL_ERROR();

    NvAssetLoaderFree(gateData);

    return true;
}

GLuint Bloom::initFBO(GLuint width, GLuint height, GLuint internalformat, GLuint format, GLuint type, GLuint mipmapped, GLuint depthformat, FBOType& fbo)
{
    GLuint status;
    int fnresult;

    fbo.depth_texture = 0;

    glGenFramebuffers(1,&(fbo.fbo));

    if(internalformat) {
        glGenTextures(1,&(fbo.color_texture));
        glBindTexture(GL_TEXTURE_2D,fbo.color_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, NULL);

        if(mipmapped) {
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
            glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        } else {
            glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        }
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo.color_texture, 0);
    }

    if(depthformat) {
        glGenRenderbuffers(1,&(fbo.depth_renderbuffer));
        glBindRenderbuffer(GL_RENDERBUFFER,fbo.depth_renderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, depthformat,width,height);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo.depth_renderbuffer);
    }

    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    fnresult = 0;
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE:
            fbo.width = width;
            fbo.height = height;
            if(internalformat) {
                LOGI("FBO created: %4ix%4i [%s fmt=0x%4x ifmt=0x%4x] %s",
                    width, height, mipmapped ? "mipmapped" : "nomipmaps", 
                    format, internalformat, depthformat ? "+ depth" : "");
            } else {
                LOGI("FBO created: %4ix%4i [%s fmt=0x0000 ifmt=0x0000] %s",
                    width, height, "nocolor  ", depthformat ? "+ depth" : "");
            }
            fnresult = 1;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            LOGI("Can't create FBO: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT error occurs");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            LOGI("Can't create FBO: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT error occurs");
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            LOGI("Can't create FBO: GL_FRAMEBUFFER_UNSUPPORTED error occurs");
            break;
        default:
            LOGI("Can't create FBO: unknown error occurs");
            break;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());

    return(fnresult);
}

GLuint Bloom::initDepthTexFBO(GLuint width, GLuint height, GLuint internalformat, GLuint format, GLuint type, GLuint mipmapped, FBOType& fbo)
{
    GLuint status;
    int fnresult;

    fbo.depth_texture = 0;

    glGenFramebuffers(1,&(fbo.fbo));

    if(internalformat) {
        glGenTextures(1,&(fbo.color_texture));
        glBindTexture(GL_TEXTURE_2D,fbo.color_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, NULL);

        if(mipmapped) {
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
            glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        } else {
            glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        }
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo.color_texture, 0);
    }

    glGenTextures(1,&(fbo.depth_texture));
    glBindTexture(GL_TEXTURE_2D,fbo.depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);

    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo.depth_texture, 0);    CHECK_GL_ERROR();

    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    fnresult = 0;
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE:
            fbo.width = width;
            fbo.height = height;
            if(internalformat) {
                LOGI("FBO created: %4ix%4i [%s fmt=0x%4x ifmt=0x%4x]",
                    width, height, mipmapped ? "mipmapped" : "nomipmaps",
                    format, internalformat);
            } else {
                LOGI("FBO created: %4ix%4i [%s fmt=0x0000 ifmt=0x0000]",
                    width, height, "nocolor  ");
            }
            fnresult = 1;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            LOGI("Can't create FBO: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT error occurs");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            LOGI("Can't create FBO: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT error occurs");
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            LOGI("Can't create FBO: GL_FRAMEBUFFER_UNSUPPORTED error occurs");
            break;
        default:
            LOGI("Can't create FBO: unknown error occurs");
            break;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());

    return(fnresult);
}

void Bloom::updateDynamics()
{
    nv::matrix4f translation_matrix;
    nv::matrix4f rotation_matrix1;
    nv::matrix4f rotation_matrix2;
    nv::matrix4f projection_matrix;

    // setting up shadow viewproj matrix
    nv::vec3f light_direction = mLightPosition;
    light_direction = nv::normalize(light_direction);

    mDynamics.mWorldLightDir = nv::inverse(m_transformer->getRotationMat()) 
        * nv::vec4f(mLightPosition, 0.0f);
    mDynamics.mWorldLightDir = nv::normalize(mDynamics.mWorldLightDir);

    nv::rotationY(rotation_matrix1, (float)atan2(-light_direction[0],light_direction[2]));
    nv::rotationX(rotation_matrix2, (float)asin(light_direction[1]));

    nv::translation(translation_matrix, 0.0f,0.0f,0.0f);

    nv::matrix4f shadow_view_matrix = rotation_matrix2 * rotation_matrix1 * translation_matrix *
        m_transformer->getRotationMat();

    const float xs = 1.0f;
    const float ys = 1.0f;
    const float zs = 1.5f;
    nv::ortho3D(projection_matrix,-xs,xs,-ys,ys, -zs, zs);

    mDynamics.mShadowMVP = projection_matrix * shadow_view_matrix;

    // setting up normal viewproj matrix
    nv::perspective(projection_matrix, FOV * 0.5f, m_width/(float)m_height, Z_NEAR, Z_FAR);
    mDynamics.mCameraMVP = projection_matrix * m_transformer->getModelViewMat();
}



NvAppBase* NvAppFactory() {
    return new Bloom();
}
