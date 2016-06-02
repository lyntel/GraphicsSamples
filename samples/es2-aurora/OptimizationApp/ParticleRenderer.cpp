//----------------------------------------------------------------------------------
// File:        es2-aurora\OptimizationApp/ParticleRenderer.cpp
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

#include "ParticleRenderer.h"
#include "Shaders.h"

ParticleRenderer::ParticleRenderer(bool isES2)
    : m_vboArray(0)
    , m_frameId(0)
    , m_eboArray(NULL)
    , m_bufferCount(2)
{
    m_particleSystem = new ParticleSystem();

    createShaders(isES2);
    createVBOs();
    createEBOs();
}

ParticleRenderer::~ParticleRenderer()
{
    delete m_particleSystem;

    deleteShaders();
    deleteVBOs();
    deleteEBOs();
}

void ParticleRenderer::createShaders(bool isES2)
 {
    m_cameraViewParticleProg = new CameraViewParticleProgram(isES2);
 }

void ParticleRenderer::deleteShaders()
{
    delete m_cameraViewParticleProg;
}

NvGLSLProgram& ParticleRenderer::getCameraViewProgram()        
{ 
    return *m_cameraViewParticleProg; 
}

void ParticleRenderer::drawPointsSorted(GLint positionAttrib, int32_t start, int32_t count)
{
    glBindBuffer(GL_ARRAY_BUFFER, m_vboArray[m_frameId]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboArray[m_frameId]);

    glVertexAttribPointer(positionAttrib, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(positionAttrib);

    // Hack for OpenGL-as-GLES; need to avoid this
#ifndef ANDROID
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_PROGRAM_POINT_SIZE);
#endif
    glDrawElements(GL_POINTS, count, GL_UNSIGNED_SHORT, (void*)(sizeof(GLushort)*start));

    glDisableVertexAttribArray(positionAttrib);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void ParticleRenderer::drawSliceCameraView(SceneInfo& scene, int32_t i)
{
    targetFBO(scene).bind(); CHECK_GL_ERROR();

    // back-to-front blending with pre-multiplied alpha
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); CHECK_GL_ERROR();

    m_cameraViewParticleProg->enable(); CHECK_GL_ERROR();

    // these uniforms are slice-invariant, so only set them for the first slice
    if (i == 0) 
        m_cameraViewParticleProg->setUniforms(scene, m_params);

    drawPointsSorted(m_cameraViewParticleProg->getPositionAndColorAttrib(), i*m_batchSize, m_batchSize);
}

void ParticleRenderer::renderParticles(SceneInfo& scene)
{
    if (!m_params.render)
        return;

    // depth-test the particles against the low-res depth buffer
    glEnable(GL_DEPTH_TEST);

    // Clear volume image only if we are rendering to a special off-screen buffer.  
    targetFBO(scene).bind();
    if (m_params.renderLowResolution)
    {
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    else
    {
        // Otherwise we are blending on top of the main scene.  We still need to clear dest alpha only.
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    glDepthMask(GL_FALSE);  // don't write depth
    glEnable(GL_BLEND);

    // render slices
    for(int32_t i = 0; i < (int32_t)m_params.numSlices; ++i)
    {
        // draw slice from camera view, sampling light buffer
        drawSliceCameraView(scene, i);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void ParticleRenderer::deleteVBOs()
{
    if (m_vboArray)
    {
        glDeleteBuffers(m_bufferCount, m_vboArray);
        m_vboArray = 0;
    }
}

void ParticleRenderer::createVBOs()
{
    deleteVBOs();

    m_vboArray = new GLuint[m_bufferCount];
    glGenBuffers(m_bufferCount, m_vboArray);

    for (int32_t i = 0; i < m_bufferCount; ++i)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_vboArray[i]);
        glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float) * getNumActive(), NULL, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        CHECK_GL_ERROR();
    }
}

void ParticleRenderer::deleteEBOs()
{
    if (m_eboArray)
    {
        glDeleteBuffers(m_bufferCount, m_eboArray);
        m_eboArray = NULL;
    }
}

void ParticleRenderer::createEBOs()
{
    deleteEBOs();

    // to avoid CPU<->GPU sync points when updating DYNAMIC buffers,
    // use an array of buffers and use the least-recently-used one each frame.
    m_eboArray = new GLuint[m_bufferCount];
    glGenBuffers(m_bufferCount, m_eboArray);

    for (int32_t i = 0; i < m_bufferCount; ++i)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboArray[i]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * getNumActive(), NULL, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        CHECK_GL_ERROR();
    }
}

void ParticleRenderer::updateEBO()
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboArray[m_frameId]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * getNumActive(), m_particleSystem->getSortedIndices(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    CHECK_GL_ERROR();
}

void ParticleRenderer::updateVBO()
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboArray[m_frameId]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(float) * getNumActive(), m_particleSystem->getPositions(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    CHECK_GL_ERROR();
}

void ParticleRenderer::updateBuffers()
{
    updateVBO();
    updateEBO();
}

void ParticleRenderer::simulate(SceneInfo& scene, float frameElapsed)
{
    m_particleSystem->simulate(frameElapsed, scene.m_viewVector, scene.m_eyePos);
    m_batchSize = getNumActive() / (int32_t)m_params.numSlices;
}
