//----------------------------------------------------------------------------------
// File:        es2-aurora\OptimizationApp/SceneRenderer.cpp
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
#include "AppExtensions.h"
#include "NvImage/NvImage.h"
#include "NvGLUtils/NvImageGL.h"

void MatrixStorage::multiply()
{
  m_modelView           = m_view*m_model;
  m_modelViewProjection = m_projection*m_modelView;
}

class ObjectSorter
{
public:
    bool operator()(const MeshObj& a, const MeshObj& b)
    {
        // Alpha test false comes first in the sort order.
        return a.m_alphaTest < b.m_alphaTest;
    }
};

SceneRenderer::SceneRenderer(bool isES2)
{
    initTimers();

    // Call this early to give it time to multi-thread init.
    m_particles = new ParticleRenderer(isES2);

    m_texStorage["floor"]       = NvImageGL::UploadTextureFromDDSFile("images/tex1.dds"); 
    m_texStorage["white_dummy"] = NvImageGL::UploadTextureFromDDSFile("images/white_dummy.dds"); 

    m_modelStorage["T34-85"] = loadModelFromFile("models/T34-85.obj", 30.0f);
    m_modelStorage["cow"]    = loadModelFromFile("models/cow.obj",    10.0f);

    const float treeOffset = 20;
    NvImage::VerticalFlip(false);
    m_texStorage["palm"]   = NvImageGL::UploadTextureFromDDSFile("images/palm.dds");
    NvImage::VerticalFlip(true);
    m_modelStorage["palm"] = loadModelFromFile("models/palm_tree.obj", 50.0f);

    // create terrain
    nv::vec3f scale(1000.0f, 100.0, 1000.0f);
    nv::vec3f translate(0.0f, -45.0, 0.0f);
    nv::matrix4f scaleMat, translateMat;
    scaleMat.set_scale(scale);
    translateMat.set_translate(translate);
    
    TerrainInput input;
    input.heightmap = "images/terrain_heightmap.dds";
    input.colormap  = "images/terrain_colormap.dds";
    input.transform = translateMat*scaleMat;
    input.subdivsX  = 128;
    input.subdivsY  = 128;
    m_pTerrain = new Terrain(input);

    CreateRandomObjectsOnLandScape(m_models, m_modelStorage, m_texStorage, treeOffset);
    std::sort(m_models.begin(), m_models.end(), ObjectSorter());

    m_opaqueColorProg = new OpaqueColorProgram(isES2);
    m_opaqueSolidDepthProg = new OpaqueDepthProgram("shaders/unshaded_solid.frag");
    m_opaqueAlphaDepthProg = new OpaqueDepthProgram("shaders/unshaded_alpha.frag");

    m_fbos = new SceneFBOs();

    m_upsampler = new Upsampler(m_fbos);

    // Disable particle self-shadowing and render all particles in one draw call (slice).
    getParticleParams()->numSlices = 1;

    m_scene.setLightVector(vec3f(-0.70710683f, 0.50000000f, 0.49999994f));
    m_scene.setLightDistance(6.f);
    m_scene.m_fbos = m_fbos;

    m_scene.m_lightAmbient = 0.15f;
    m_scene.m_lightDiffuse = 0.85f;

    m_statsCountdown = STATS_FRAMES;
}

SceneRenderer::~SceneRenderer()
{
    delete m_fbos;
    delete m_opaqueColorProg;
    delete m_opaqueSolidDepthProg;
    delete m_opaqueAlphaDepthProg;
    delete m_particles;
}

void SceneRenderer::initTimers()
{
    for (int32_t i = 0; i < GPU_TIMER_COUNT; ++i)
    {
        m_GPUTimers[i].init();
    }

    for (int32_t i = 0; i < CPU_TIMER_COUNT; ++i)
    {
        m_CPUTimers[i].init();
    }
}

static bool dumpMatrices = false;

void printMatrixLog(const char* label, nv::matrix4f matrix)
{
    if (dumpMatrices)
    {
        LOGI("%s\n", label);
        for (int32_t i = 0; i < 4; i++) {
            LOGI("%f\t%f\t%f\t%f\n", matrix._array[i], matrix._array[i+4], matrix._array[i+8], matrix._array[i+12]);
        }
    }
}

void SceneRenderer::drawScene(NvGLSLProgram& a_proc, const MatrixStorage& mats, RenderAlphaTest doAlpha, bool a_renderDepth)
{
    CHECK_GL_ERROR();

    MatrixStorage ncMats = mats;

    GLint positionAttrHandle = a_proc.getAttribLocation("g_Position");
    GLint normalAttrHandle   = a_proc.getAttribLocation("g_Normal", false);
    GLint texCoordAttrHandle = a_proc.getAttribLocation("g_TexCoord", false);
    CHECK_GL_ERROR();

    // these matrices cam be set once
    a_proc.setUniformMatrix4fv("g_ProjectionMatrix",          ncMats.m_projection._array); 
    printMatrixLog("viewMatrix", ncMats.m_view);    
    printMatrixLog("set g_ProjectionMatrix", ncMats.m_projection);
    a_proc.setUniform1f("g_lightAmbient", m_scene.m_lightAmbient);
    a_proc.setUniform1f("g_lightDiffuse", m_scene.m_lightDiffuse);
    a_proc.setUniform3f("g_lightDirection", m_scene.m_lightVector[0], m_scene.m_lightVector[1], m_scene.m_lightVector[2]);
    CHECK_GL_ERROR();

  // Note that we are drawing the terrain first which is a sub-optimal rendering order and a deliberate mistake.
  if (!a_renderDepth && (doAlpha & RENDER_SOLID))
  { 
    nv::matrix4f modelMatrix; 
    modelMatrix.make_identity();
    modelMatrix.set_scale(nv::vec3f(1.0f, 1.0, 1.0f));

    ncMats.m_model = modelMatrix;
    ncMats.multiply();
    
    CHECK_GL_ERROR();
    a_proc.setUniformMatrix4fv("g_ModelViewMatrix",           ncMats.m_modelView._array);
    a_proc.setUniformMatrix4fv("g_ModelViewProjectionMatrix", ncMats.m_modelViewProjection._array);
    a_proc.setUniformMatrix4fv("g_LightModelViewMatrix",      m_scene.m_lightView._array);

    CHECK_GL_ERROR();
    a_proc.setUniform4f("g_color", 1.0f, 1.0f, 1.0f, 1.0f);

    CHECK_GL_ERROR();
    a_proc.bindTexture2D("g_texture", 0, m_pTerrain->getColorTex());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECK_GL_ERROR();
    m_pTerrain->Draw(positionAttrHandle, normalAttrHandle, texCoordAttrHandle);
    CHECK_GL_ERROR();
  }

  // render objects
  for(uint32_t i = 0; i < m_models.size(); i++)
  {
    MeshObj& mesh = m_models[i];

    if (doAlpha == RENDER_ALL || (doAlpha == RENDER_ALPHA && mesh.m_alphaTest) || (doAlpha == RENDER_SOLID && !mesh.m_alphaTest))
    {
        ncMats.m_model = mesh.m_modelMatrix;
        ncMats.multiply();

        CHECK_GL_ERROR();
        a_proc.setUniformMatrix4fv("g_ModelViewMatrix",           ncMats.m_modelView._array);
        a_proc.setUniformMatrix4fv("g_ModelViewProjectionMatrix", ncMats.m_modelViewProjection._array);
        printMatrixLog("set modelViewProjection", ncMats.m_modelViewProjection);
        CHECK_GL_ERROR();
    
        if (doAlpha & RENDER_ALPHA) {
            a_proc.bindTexture2D("g_texture", 0, mesh.m_texId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        if(!a_renderDepth)
        {
          a_proc.setUniform4f("g_color", mesh.m_color.x, mesh.m_color.y, mesh.m_color.z, mesh.m_color.w);
          a_proc.setUniform1f("g_lightSpecular", mesh.m_specularValue);
        }
    
        if (mesh.m_cullFacing)
            glEnable(GL_CULL_FACE);
        else
            glDisable(GL_CULL_FACE);

        CHECK_GL_ERROR();
        mesh.m_pModelData->drawElements(positionAttrHandle, normalAttrHandle, texCoordAttrHandle); CHECK_GL_ERROR();
      }
  }
  glEnable(GL_CULL_FACE);
}

// render the opaque geometry to the depth buffer
void SceneRenderer::renderSceneDepth(const MatrixStorage& mats, NvWritableFB* depthFbo)
{
    // bind the FBO and set the viewport to the FBO resolution
    depthFbo->bind();
    CHECK_GL_ERROR();

    // depth-only pass, disable color writes
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // enable depth testing and depth writes
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    CHECK_GL_ERROR();

    // clear depths to 1.0
    glClear(GL_DEPTH_BUFFER_BIT);

    // draw the geometry with a dummy fragment shader
    m_opaqueSolidDepthProg->enable();
    CHECK_GL_ERROR();
    m_opaqueSolidDepthProg->setUniforms(m_scene);
    CHECK_GL_ERROR();
    drawScene(*m_opaqueSolidDepthProg, mats, RENDER_SOLID);

    m_opaqueAlphaDepthProg->enable();
    CHECK_GL_ERROR();
    m_opaqueAlphaDepthProg->setUniforms(m_scene);
    CHECK_GL_ERROR();
    drawScene(*m_opaqueAlphaDepthProg, mats, RENDER_ALPHA);

    // restore color writes
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    CHECK_GL_ERROR();
}

void SceneRenderer::downsampleSceneDepth(const MatrixStorage& mats, NvSimpleFBO *srcFbo, NvWritableFB *dstFbo)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFbo->fbo);        CHECK_GL_ERROR();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFbo->fbo);        CHECK_GL_ERROR();

    glBlitFramebufferFunc(0, 0, srcFbo->width, srcFbo->height,
                        0, 0, dstFbo->width, dstFbo->height,
                        GL_DEPTH_BUFFER_BIT, GL_NEAREST);        CHECK_GL_ERROR();
}

// initialize the depth buffer to depth-test the low-res particles against
void SceneRenderer::renderLowResSceneDepth(const MatrixStorage& mats)
{
    if (m_params.useDepthPrepass && glBlitFramebufferFunc)
        downsampleSceneDepth(mats, m_fbos->m_sceneFbo, m_fbos->m_particleFbo);

    if (getParticleParams()->render)
        renderSceneDepth(mats, m_fbos->m_particleFbo);

    CHECK_GL_ERROR();
}

// render the colors of the opaque geometry, receiving shadows from the particles
void SceneRenderer::renderFullResSceneColor(const MatrixStorage& mats)
{
    glClearColor(m_params.backgroundColor.x, m_params.backgroundColor.y, m_params.backgroundColor.z, 0.0f);
    glEnable(GL_DEPTH_TEST);

    m_fbos->m_sceneFbo->bind();
    
    m_opaqueColorProg->enable();
    m_opaqueColorProg->setUniforms(m_scene);
    CHECK_GL_ERROR();

    if (m_params.useDepthPrepass)
    {
        // if we are using the depth pre-pass strategy, then re-use the full-resolution depth buffer
        // initialized earlier and perform a z-equal pass against it with depth writes disabled.

        glDepthFunc(GL_EQUAL);
        glDepthMask(GL_FALSE);

        glClear(GL_COLOR_BUFFER_BIT);
        CHECK_GL_ERROR();

        drawScene(*m_opaqueColorProg, mats, RENDER_ALL);

        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
    }
    else
    {
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        CHECK_GL_ERROR();

        drawScene(*m_opaqueColorProg, mats, RENDER_ALL);
    }
}

void SceneRenderer::renderParticles(const MatrixStorage& mats)
{
    MatrixStorage matsCopy = mats;
    matsCopy.m_model.make_identity();
    matsCopy.multiply();
    CHECK_GL_ERROR();
    NvGLSLProgram& prog = m_particles->getCameraViewProgram();
    prog.enable();
    prog.setUniformMatrix4fv("g_LightModelViewMatrix", m_scene.m_lightView._array); CHECK_GL_ERROR();
    m_particles->renderParticles(m_scene);
}

void SceneRenderer::setupMatrices(MatrixStorage& matContainer)
{
    matContainer.m_view       = m_scene.m_eyeView;
    matContainer.m_projection = m_scene.m_eyeProj;
}

void SceneRenderer::updateFrame(float frameElapsed)
{
    if (m_particles->getParams().render)
    {
        m_scene.calcVectors();
        m_particles->simulate(m_scene, frameElapsed);
        m_particles->updateBuffers();
    }
}

void SceneRenderer::renderFrame()
{
    m_fbos->updateBuffers();

    // Update screen FBO
    glGetIntegerv(0x8CA6, (GLint*)&(m_fbos->m_backBufferFbo->fbo));

    CPU_TIMER_SCOPE(CPU_TIMER_TOTAL);
    GPU_TIMER_SCOPE(GPU_TIMER_TOTAL);

    MatrixStorage mats;
    setupMatrices(mats);
    
    {
        // render a full-screen depth pass.
        CPU_TIMER_SCOPE(CPU_TIMER_SCENE_DEPTH);
        GPU_TIMER_SCOPE(GPU_TIMER_SCENE_DEPTH);
        if (m_params.useDepthPrepass)
            renderSceneDepth(mats, m_fbos->m_sceneFbo);

        // render scene depth to buffer for particle to be depth tested against
        // This may just down-sample the above depth pre-pass.
        renderLowResSceneDepth(mats);
        CHECK_GL_ERROR();
    }

    {
        // the opaque colors need to be rendered after the particles
        //LOGI("OE renderFullResSceneColor\n");
        CPU_TIMER_SCOPE(CPU_TIMER_SCENE_COLOR);
        GPU_TIMER_SCOPE(GPU_TIMER_SCENE_COLOR);
        renderFullResSceneColor(mats);
        CHECK_GL_ERROR();
    }
    
    if (m_particles->getParams().render)
    {
        CPU_TIMER_SCOPE(CPU_TIMER_PARTICLES);
        GPU_TIMER_SCOPE(GPU_TIMER_PARTICLES);
        renderParticles(mats);
        CHECK_GL_ERROR();

        if (m_particles->getParams().renderLowResolution)
        {
            // upsample the particles & composite them on top of the opaque scene colors
            CPU_TIMER_SCOPE(CPU_TIMER_UPSAMPLE_PARTICLES);
            GPU_TIMER_SCOPE(GPU_TIMER_UPSAMPLE_PARTICLES);
            m_upsampler->upsampleParticleColors(*m_fbos->m_sceneFbo);
            CHECK_GL_ERROR();
        }
    }
    
    {
        // final bilinear upsampling from scene resolution to backbuffer resolution
        CPU_TIMER_SCOPE(CPU_TIMER_UPSAMPLE_SCENE);
        GPU_TIMER_SCOPE(GPU_TIMER_UPSAMPLE_SCENE);
        m_upsampler->upsampleSceneColors(*m_fbos->m_backBufferFbo);
        CHECK_GL_ERROR();
    }

    m_particles->swapBuffers();
}

// Have to deal with the fact that not all requested timer start/stop pairs
// will have completed when we request results.  So we need to normalize for
// however many we've actually gotten back.
static float ComputePercentage(NvGPUTimer& timer, float meanTotal)
{
    int32_t startStop = timer.getStartStopCycles();
    return (startStop > 0) 
        ? (100.f * timer.getScaledCycles() / (startStop * meanTotal)) : 0.0f;
}

bool SceneRenderer::stats(char* buffer, int32_t size)
{
    if (!m_statsCountdown) {
        float meanGPUTotal = m_GPUTimers[GPU_TIMER_TOTAL].getScaledCycles() /
            m_GPUTimers[GPU_TIMER_TOTAL].getStartStopCycles();

        sprintf(buffer, 
            "GPU Scene depth: %5.1f%%\n"
            "CPU Scene depth: %5.1f%%\n"
            "GPU Scene color: %5.1f%%\n"
            "CPU Scene color: %5.1f%%\n"
            "GPU Scene upsamp: %5.1f%%\n"
            "CPU Scene upsamp: %5.1f%%\n"
            "GPU Particles: %5.1f%%\n"
            "CPU Particles: %5.1f%%\n"
            "GPU Particles upsamp: %5.1f%%\n"
            "CPU Particles upsamp: %5.1f%%\n",
            ComputePercentage(m_GPUTimers[GPU_TIMER_SCENE_DEPTH], meanGPUTotal),
            100.f * m_CPUTimers[CPU_TIMER_SCENE_DEPTH].getScaledCycles()        / m_CPUTimers[CPU_TIMER_TOTAL].getScaledCycles(),
            ComputePercentage(m_GPUTimers[GPU_TIMER_SCENE_COLOR], meanGPUTotal),
            100.f * m_CPUTimers[CPU_TIMER_SCENE_COLOR].getScaledCycles()        / m_CPUTimers[CPU_TIMER_TOTAL].getScaledCycles(),
            ComputePercentage(m_GPUTimers[GPU_TIMER_UPSAMPLE_SCENE], meanGPUTotal),
            100.f * m_CPUTimers[CPU_TIMER_UPSAMPLE_SCENE].getScaledCycles()     / m_CPUTimers[CPU_TIMER_TOTAL].getScaledCycles(),
            ComputePercentage(m_GPUTimers[GPU_TIMER_PARTICLES], meanGPUTotal),
            100.f * m_CPUTimers[CPU_TIMER_PARTICLES].getScaledCycles()          / m_CPUTimers[CPU_TIMER_TOTAL].getScaledCycles(),
            ComputePercentage(m_GPUTimers[GPU_TIMER_UPSAMPLE_PARTICLES], meanGPUTotal),
            100.f * m_CPUTimers[CPU_TIMER_UPSAMPLE_PARTICLES].getScaledCycles() / m_CPUTimers[CPU_TIMER_TOTAL].getScaledCycles());

        for (int32_t i = 0; i < GPU_TIMER_COUNT; ++i)
        {
            m_GPUTimers[i].reset();
        }

        for (int32_t i = 0; i < CPU_TIMER_COUNT; ++i)
        {
            m_CPUTimers[i].reset();
        }
    
        m_statsCountdown = STATS_FRAMES;
        return true;
    } else {
        m_statsCountdown--;
        return false;
    }
}