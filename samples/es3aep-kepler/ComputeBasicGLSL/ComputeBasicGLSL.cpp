//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ComputeBasicGLSL/ComputeBasicGLSL.cpp
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
#include "ComputeBasicGLSL.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvImage/NvImage.h"
#include "NV/NvLogs.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NvUI/NvTweakBar.h"

ComputeBasicGLSL::ComputeBasicGLSL()
{
    m_framerate = new NvFramerateCounter(this);
    m_aspectRatio = 1.0f;
    m_enableFilter = true;

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

ComputeBasicGLSL::~ComputeBasicGLSL()
{
    LOGI("ComputeBasicGLSL: destroyed\n");
}

void ComputeBasicGLSL::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionES3_1();
}


void ComputeBasicGLSL::initUI(void) 
{
    if (mTweakBar) {
        mTweakBar->addValue("Enable filter", m_enableFilter);
    }
}


void ComputeBasicGLSL::initRendering(void) 
{
    NvAssetLoaderAddSearchPath("es3aep-kepler/ComputeBasicGLSL");

    if (!requireMinAPIVersion(NvGLAPIVersionES3_1()))
        return;

    {
        NvScopedShaderPrefix switched(
        (getGLContext()->getConfiguration().apiVer.api == NvGLAPI::GL)
        ? "#version 430\n" : "#version 310 es\n");

        //init shaders
        m_blitProg = NvGLSLProgram::createFromFiles("shaders/plain.vert", "shaders/plain.frag");

        m_computeProg = new NvGLSLProgram;
        int32_t len;
        NvGLSLProgram::ShaderSourceItem sources[1];
        sources[0].type = GL_COMPUTE_SHADER;
        sources[0].src = NvAssetLoaderRead("shaders/invert.glsl", len);
        m_computeProg->setSourceFromStrings(sources, 1);
        NvAssetLoaderFree((char*)sources[0].src);
    }

    //load input texture - this is a normal, "mutable" texture
    m_sourceImage = NvImage::CreateFromDDSFile("textures/flower1024.dds");
    GLint w = m_sourceImage->getWidth();
    GLint h = m_sourceImage->getHeight();
    GLint intFormat = m_sourceImage->getInternalFormat();
    GLint format = m_sourceImage->getFormat();
    GLint type = m_sourceImage->getType();

    // Image must be immutable in order to be used with glBindImageTexture
    // So we copy the mutable texture to an immutable texture
    glGenTextures(1, &m_sourceTexture );
    glBindTexture(GL_TEXTURE_2D, m_sourceTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, intFormat, w, h );
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, format, type, m_sourceImage->getLevel(0));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    //create output texture with same size and format as input 
    // Image must be immutable in order to be used with glBindImageTexture
    glGenTextures(1, &m_resultTexture );
    glBindTexture(GL_TEXTURE_2D, m_resultTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, intFormat, w, h );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    CHECK_GL_ERROR();

    glBindTexture(GL_TEXTURE_2D, 0);

    }


void ComputeBasicGLSL::reshape(int32_t width, int32_t height)
{
    glViewport( 0, 0, (GLint) width, (GLint) height );
    m_aspectRatio = (float)height/(float)width;

    CHECK_GL_ERROR();
}

#define WORKGROUP_SIZE 16
void ComputeBasicGLSL::runComputeFilter(GLuint inputTex, GLuint outputTex, int width, int height)
{
    glUseProgram( m_computeProg->getProgram() );

    glBindImageTexture(0, inputTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8); 
    glBindImageTexture(1, outputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    glDispatchCompute(width/WORKGROUP_SIZE, height/WORKGROUP_SIZE, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);  
}

void ComputeBasicGLSL::drawImage(GLuint texture)
{
    float const vertexPosition[] = { 
        m_aspectRatio, -1.0f, 
        -m_aspectRatio, -1.0f,
        m_aspectRatio, 1.0f, 
        -m_aspectRatio, 1.0f};

    float const textureCoord[] = { 
        1.0f, 0.0f, 
        0.0f, 0.0f, 
        1.0f, 1.0f, 
        0.0f, 1.0f };

    glClearColor(0.2f, 0.0f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_blitProg->getProgram());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glUniform1i(m_blitProg->getUniformLocation("uSourceTex"), 0);
    int aPosCoord = m_blitProg->getAttribLocation("aPosition");
    int aTexCoord = m_blitProg->getAttribLocation("aTexCoord");

    glVertexAttribPointer(aPosCoord, 2, GL_FLOAT, GL_FALSE, 0, vertexPosition);
    glVertexAttribPointer(aTexCoord, 2, GL_FLOAT, GL_FALSE, 0, textureCoord);
    glEnableVertexAttribArray(aPosCoord);
    glEnableVertexAttribArray(aTexCoord);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    CHECK_GL_ERROR();

}


void ComputeBasicGLSL::draw(void)
{
    if(m_enableFilter){
        runComputeFilter(m_sourceTexture, m_resultTexture, m_sourceImage->getWidth(), m_sourceImage->getHeight());
        drawImage(m_resultTexture);

    }else{
        drawImage(m_sourceTexture);
    }

    // print fps
    if (m_framerate->nextFrame()) {
        LOGI("fps: %.2f", m_framerate->getMeanFramerate());
    }
}

bool ComputeBasicGLSL::handleGamepadChanged(uint32_t changedPadFlags) {
    if (changedPadFlags) {
        NvGamepad* pad = getPlatformContext()->getGamepad();
        if (!pad) return false;

        LOGI("gamepads: 0x%08x", changedPadFlags);
    }

    return false;
}


NvAppBase* NvAppFactory() {
    return new ComputeBasicGLSL();
}
