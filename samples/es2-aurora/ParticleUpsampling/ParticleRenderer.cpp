//----------------------------------------------------------------------------------
// File:        es2-aurora\ParticleUpsampling/ParticleRenderer.cpp
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

#include "NV/NvLogs.h"
#include "ParticleRenderer.h"
#include "Shaders.h"

ParticleRenderer::ParticleRenderer(bool isGL)
    : m_vbo(0)
    , m_frameId(0)
    , m_eboArray(NULL)
    , m_eboCount(2)
    , m_isGL(isGL)
{
    m_particleSystem = new ParticleSystem();

    createShaders();
    createVBO();
    createEBOs();
}

ParticleRenderer::~ParticleRenderer()
{
    delete m_particleSystem;

    deleteShaders();
    deleteVBO();
    deleteEBOs();
}

void ParticleRenderer::createShaders()
 {
    m_lightViewParticleProg         = new LightViewParticleProgram();
    m_cameraViewParticleProg     = new CameraViewParticleProgram();
 }

void ParticleRenderer::deleteShaders()
{
    delete m_lightViewParticleProg;
    delete m_cameraViewParticleProg;
}

#ifndef GL_POINT_SPRITE
#define GL_POINT_SPRITE                   0x8861
#endif

#ifndef GL_PROGRAM_POINT_SIZE
#define GL_PROGRAM_POINT_SIZE                   0x8642
#endif

void ParticleRenderer::drawPointsSorted(GLint positionAttrib, int start, int count)
{
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboArray[m_frameId]);

    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(positionAttrib);

        CHECK_GL_ERROR();
    if (m_isGL) {
        glEnable(GL_POINT_SPRITE);
        glEnable(GL_PROGRAM_POINT_SIZE);
    }
    glDrawElements(GL_POINTS, count, GL_UNSIGNED_SHORT, (void*)(sizeof(GLushort)*start));
        CHECK_GL_ERROR();

    glDisableVertexAttribArray(positionAttrib);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void ParticleRenderer::drawSliceCameraView(SceneInfo& scene, int i)
{
    scene.m_fbos->m_particleFbo->bind();

    if (scene.m_invertedView)
    {
        // front-to-back blending
        glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
    }
    else
    {
        // back-to-front blending with pre-multiplied alpha
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    m_cameraViewParticleProg->enable();

    // these uniforms are slice-invariant, so only set them for the first slice
    if (i == 0) m_cameraViewParticleProg->setUniforms(scene, m_params);

    drawPointsSorted(m_cameraViewParticleProg->getPositionAttrib(), i*m_batchSize, m_batchSize);
}

void ParticleRenderer::drawSliceLightView(SceneInfo& scene, int i)
{
    scene.m_fbos->m_lightFbo->bind();

    // back-to-front alpha blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_lightViewParticleProg->enable();

    // these uniforms are slice-invariant, so only set them for the first slice
    if (i == 0) m_lightViewParticleProg->setUniforms(scene, m_params);

    drawPointsSorted(m_lightViewParticleProg->getPositionAttrib(), i*m_batchSize, m_batchSize);
}

void ParticleRenderer::renderParticles(SceneInfo& scene)
{
    // depth-test the particles against the low-res depth buffer
    glEnable(GL_DEPTH_TEST);

    glDepthMask(GL_FALSE);  // don't write depth
    glEnable(GL_BLEND);

    // render slices
    for(uint32_t i = 0; i < m_params.numSlices; ++i)
    {
        // draw slice from camera view, sampling light buffer
        CHECK_GL_ERROR();
        drawSliceCameraView(scene, i);
        CHECK_GL_ERROR();

        if (m_params.renderShadows)
        {
            // draw slice from light view to light buffer, accumulating shadows
            CHECK_GL_ERROR();
            drawSliceLightView(scene, i);
            CHECK_GL_ERROR();
        }
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void ParticleRenderer::deleteVBO()
{
    if (m_vbo)
    {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
}

void ParticleRenderer::createVBO()
{
    deleteVBO();

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(float) * getNumActive(), m_particleSystem->getPositions(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CHECK_GL_ERROR();
}

void ParticleRenderer::deleteEBOs()
{
    if (m_eboArray)
    {
        glDeleteBuffers(m_eboCount, m_eboArray);
        m_eboArray = NULL;
    }
}

void ParticleRenderer::createEBOs()
{
    deleteEBOs();

    // to avoid CPU<->GPU sync points when updating DYNAMIC buffers,
    // use an array of buffers and use the least-recently-used one each frame.
    m_eboArray = new GLuint[m_eboCount];
    glGenBuffers(m_eboCount, m_eboArray);

    for (int i = 0; i < m_eboCount; ++i)
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

void ParticleRenderer::depthSort(SceneInfo& scene)
{
    m_particleSystem->depthSort(scene.m_halfVector);

    m_batchSize = getNumActive() / (int)m_params.numSlices;
}
