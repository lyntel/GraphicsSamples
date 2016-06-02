//----------------------------------------------------------------------------------
// File:        es2-aurora\ParticleUpsampling/SceneRenderer.cpp
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

#include "SceneRenderer.h"
#include "Shaders.h"
#include "NvGLUtils/NvModelGL.h"
#include "NvAssetLoader/NvAssetLoader.h"

SceneRenderer::SceneRenderer(bool isGL)
: m_model(NULL)
{
    loadModel();
    initTimers();

    m_opaqueColorProg = new OpaqueColorProgram();
    m_opaqueDepthProg = new OpaqueDepthProgram();

    m_fbos = new SceneFBOs();
    m_particles = new ParticleRenderer(isGL);
    m_upsampler = new Upsampler(m_fbos, isGL);

    memset(&m_scene, 0, sizeof(m_scene));
    m_scene.setLightVector(vec3f(-0.70710683f, 0.50000000f, 0.49999994f));
    m_scene.setLightDistance(6.f);
    m_scene.m_fbos = m_fbos;
}

SceneRenderer::~SceneRenderer()
{
    delete m_fbos;
    delete m_opaqueColorProg;
    delete m_opaqueDepthProg;
    delete m_particles;
}

void SceneRenderer::initTimers()
{
#if ENABLE_GPU_TIMERS
    for (int i = 0; i < GPU_TIMER_COUNT; ++i)
    {
        m_GPUTimers[i].init();
    }
#endif
#if ENABLE_CPU_TIMERS
    for (int i = 0; i < CPU_TIMER_COUNT; ++i)
    {
        m_CPUTimers[i].init();
    }
#endif
}

void SceneRenderer::loadModelFromData(char *fileData)
{
	m_model = NvModelGL::CreateFromObj(
		(uint8_t *)fileData, 1.0f, true, false);
}

void SceneRenderer::loadModel()
{
    // load model
    int32_t length;
    char *modelData = NvAssetLoaderRead("models/cow.obj", length);

    loadModelFromData(modelData);

    NvAssetLoaderFree(modelData);
}

void SceneRenderer::drawModel(GLint positionAttrib, GLint normalAttrib)
{
    if (!m_params.drawModel)
    {
        return;
    }

    if (normalAttrib >= 0)
    {
        m_model->drawElements(positionAttrib, normalAttrib);
    }
    else
    {
        m_model->drawElements(positionAttrib);
    }
}

void SceneRenderer::drawFloor(GLint positionAttrib, GLint normalAttrib)
{
    float s =  4.f;
    float y = -1.f;

    // world-space positions and normals
    float vertices[] =
    {
        -s, y, -s,     0.f, 1.f, 0.f,
         s, y, -s,  0.f, 1.f, 0.f,
         s, y, s,   0.f, 1.f, 0.f,
        -s, y, s,   0.f, 1.f, 0.f,
    };
    GLushort indices[] =
    {
        2,1,0,
        3,2,0
    };

    if (normalAttrib >= 0)
    {
        glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), vertices);
        glEnableVertexAttribArray(positionAttrib);

        glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), &vertices[3]);
        glEnableVertexAttribArray(normalAttrib);

        glDrawElements(GL_TRIANGLES, sizeof(indices)/sizeof(indices[0]), GL_UNSIGNED_SHORT, indices);

        glDisableVertexAttribArray(positionAttrib);
        glDisableVertexAttribArray(normalAttrib);
    }
    else
    {
        glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), vertices);
        glEnableVertexAttribArray(positionAttrib);

        glDrawElements(GL_TRIANGLES, sizeof(indices)/sizeof(indices[0]), GL_UNSIGNED_SHORT, indices);

        glDisableVertexAttribArray(positionAttrib);
    }
}

void SceneRenderer::drawScene(GLint positionAttrib, GLint normalAttrib)
{
    drawFloor(positionAttrib, normalAttrib);

    drawModel(positionAttrib, normalAttrib);
}

// render the opaque geometry to the depth buffer
void SceneRenderer::renderSceneDepth(NvSimpleFBO* depthFbo)
{
    // bind the FBO and set the viewport to the FBO resolution
    depthFbo->bind();

    // depth-only pass, disable color writes
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // enable depth testing and depth writes
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    // clear depths to 1.0
    glClear(GL_DEPTH_BUFFER_BIT);

    // draw the geometry with a dummy fragment shader
    m_opaqueDepthProg->enable();
    m_opaqueDepthProg->setUniforms(m_scene);
    drawScene(m_opaqueDepthProg->getPositionAttrib());

    // restore color writes
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void SceneRenderer::downsampleSceneDepth(NvSimpleFBO *srcFbo, NvSimpleFBO *dstFbo)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFbo->fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFbo->fbo);

    glBlitFramebuffer(0, 0, srcFbo->width, srcFbo->height,
                        0, 0, dstFbo->width, dstFbo->height,
                        GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

// initialize the depth buffer to depth-test the low-res particles against
void SceneRenderer::renderLowResSceneDepth()
{
    // if the scene-resolution depth buffer can be used as a depth pre-pass
    // to speedup the forward shading passes, then it may make sense to
    // render the opaque geometry in full resolution first, and then
    // downsample the full-resolution depths to the particle resolution.

    if (m_params.useDepthPrepass)
    {
        renderSceneDepth(m_fbos->m_sceneFbo);

        downsampleSceneDepth(m_fbos->m_sceneFbo, m_fbos->m_particleFbo);
    }
    else
    {
        renderSceneDepth(m_fbos->m_particleFbo);
    }
}

// render the colors of the opaque geometry, receiving shadows from the particles
void SceneRenderer::renderFullResSceneColor()
{
    glClearColor(m_params.backgroundColor.x, m_params.backgroundColor.y, m_params.backgroundColor.z, 0.0f);

    glEnable(GL_DEPTH_TEST);

    m_fbos->m_sceneFbo->bind();

    m_opaqueColorProg->enable();
    m_opaqueColorProg->setUniforms(m_scene);

    if (m_params.useDepthPrepass)
    {
        // if we are using the depth pre-pass strategy, then re-use the full-resolution depth buffer
        // initialized earlier and perform a z-equal pass against it with depth writes disabled.

        glDepthFunc(GL_EQUAL);
        glDepthMask(GL_FALSE);

        glClear(GL_COLOR_BUFFER_BIT);

        drawScene(m_opaqueColorProg->getPositionAttrib(), m_opaqueColorProg->getNormalAttrib());

        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
    }
    else
    {
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        drawScene(m_opaqueColorProg->getPositionAttrib(), m_opaqueColorProg->getNormalAttrib());
    }
}

void SceneRenderer::renderFrame()
{
    CPU_TIMER_SCOPE(CPU_TIMER_TOTAL);
    GPU_TIMER_SCOPE(GPU_TIMER_TOTAL);
    CHECK_GL_ERROR();

    GLuint prevFBO = 0;
    // Enum has MANY names based on extension/version
    // but they all map to 0x8CA6
    glGetIntegerv(0x8CA6, (GLint*)&prevFBO);

    {
        CPU_TIMER_SCOPE(CPU_TIMER_SORT);
        m_scene.calcVectors();
        m_particles->depthSort(m_scene);
    }

    CHECK_GL_ERROR();
    {
        CPU_TIMER_SCOPE(CPU_TIMER_UPDATE_EBO);
        m_particles->updateEBO();
        CHECK_GL_ERROR();
    }
    
    {
        // render scene depth to buffer for particle to be depth tested against
        CPU_TIMER_SCOPE(CPU_TIMER_SCENE_DEPTH);
        GPU_TIMER_SCOPE(GPU_TIMER_SCENE_DEPTH);
        renderLowResSceneDepth();
        CHECK_GL_ERROR();
    }

    // clear light buffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbos->m_lightFbo->fbo);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

        CHECK_GL_ERROR();
    // clear volume image
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbos->m_particleFbo->fbo);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
        CHECK_GL_ERROR();

    
    {
        CPU_TIMER_SCOPE(CPU_TIMER_PARTICLES);
        GPU_TIMER_SCOPE(GPU_TIMER_PARTICLES);
        m_particles->renderParticles(m_scene);
        CHECK_GL_ERROR();
    }
    
    {
        CPU_TIMER_SCOPE(CPU_TIMER_SCENE_COLOR);
        GPU_TIMER_SCOPE(GPU_TIMER_SCENE_COLOR);
        // the opaque colors need to be rendered after the particles
        // for the particles to cast shadows on the opaque scene
        renderFullResSceneColor();
        CHECK_GL_ERROR();
    }
    
    {
        CPU_TIMER_SCOPE(CPU_TIMER_UPSAMPLE_PARTICLES);
        GPU_TIMER_SCOPE(GPU_TIMER_UPSAMPLE_PARTICLES);
        // upsample the particles & composite them on top of the opaque scene colors
        m_upsampler->upsampleParticleColors(m_scene);
        CHECK_GL_ERROR();
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);

    {
        CPU_TIMER_SCOPE(CPU_TIMER_UPSAMPLE_SCENE);
        GPU_TIMER_SCOPE(GPU_TIMER_UPSAMPLE_SCENE);
        // final bilinear upsampling from scene resolution to backbuffer resolution
        m_upsampler->upsampleSceneColors(m_scene);
        CHECK_GL_ERROR();
    }

    m_particles->swapBuffers();
    stats();
}

void SceneRenderer::stats()
{
#if ENABLE_GPU_TIMERS
    LOGI("GPU: %f %f %f %f %f",
        100.f * m_GPUTimers[GPU_TIMER_SCENE_DEPTH].getScaledCycles()        / m_GPUTimers[GPU_TIMER_TOTAL].getScaledCycles(),
        100.f * m_GPUTimers[GPU_TIMER_SCENE_COLOR].getScaledCycles()        / m_GPUTimers[GPU_TIMER_TOTAL].getScaledCycles(),
        100.f * m_GPUTimers[GPU_TIMER_PARTICLES].getScaledCycles()          / m_GPUTimers[GPU_TIMER_TOTAL].getScaledCycles(),
        100.f * m_GPUTimers[GPU_TIMER_UPSAMPLE_PARTICLES].getScaledCycles() / m_GPUTimers[GPU_TIMER_TOTAL].getScaledCycles(),
        100.f * m_GPUTimers[GPU_TIMER_UPSAMPLE_SCENE].getScaledCycles()     / m_GPUTimers[GPU_TIMER_TOTAL].getScaledCycles());

    for (int32_t i = 0; i < GPU_TIMER_COUNT; ++i)
    {
        m_GPUTimers[i].reset();
    }
#endif

#if ENABLE_CPU_TIMERS
    LOGI("CPU: %f %f %f %f %f %f %f",
        100.f * m_CPUTimers[CPU_TIMER_SORT].getScaledCycles()               / m_CPUTimers[CPU_TIMER_TOTAL].getScaledCycles(),
        100.f * m_CPUTimers[CPU_TIMER_UPDATE_EBO].getScaledCycles()         / m_CPUTimers[CPU_TIMER_TOTAL].getScaledCycles(),
        100.f * m_CPUTimers[CPU_TIMER_SCENE_DEPTH].getScaledCycles()        / m_CPUTimers[CPU_TIMER_TOTAL].getScaledCycles(),
        100.f * m_CPUTimers[CPU_TIMER_PARTICLES].getScaledCycles()          / m_CPUTimers[CPU_TIMER_TOTAL].getScaledCycles(),
        100.f * m_CPUTimers[CPU_TIMER_SCENE_COLOR].getScaledCycles()        / m_CPUTimers[CPU_TIMER_TOTAL].getScaledCycles(),
        100.f * m_CPUTimers[CPU_TIMER_UPSAMPLE_PARTICLES].getScaledCycles() / m_CPUTimers[CPU_TIMER_TOTAL].getScaledCycles(),
        100.f * m_CPUTimers[CPU_TIMER_UPSAMPLE_SCENE].getScaledCycles()     / m_CPUTimers[CPU_TIMER_TOTAL].getScaledCycles());

    for (int32_t i = 0; i < CPU_TIMER_COUNT; ++i)
    {
        m_CPUTimers[i].reset();
    }
#endif
}
