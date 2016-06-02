//----------------------------------------------------------------------------------
// File:        gl4-maxwell\BlendedAA/BlendedAA.cpp
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
#include "BlendedAA.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NV/NvMath.h"
#include "NvAppBase/NvInputTransformer.h"
#include "KHR/khrplatform.h"
#include "NV/NvPlatformGL.h"

const char* model_files[2]={"models/loop","models/triangle"};

#define Z_NEAR 0.4f
#define Z_FAR 100.0f
#define FOV 3.14f*0.5f

// manually defining new functions here since they aren't in GLEW yet
typedef void (KHRONOS_APIENTRY *PFNGLRASTERSAMPLESEXTPROC)(GLuint, GLboolean);
typedef void (KHRONOS_APIENTRY *PFNGLCOVERAGEMODULATIONVPROC)(GLenum);
PFNGLRASTERSAMPLESEXTPROC glRasterSamplesEXT;
PFNGLCOVERAGEMODULATIONVPROC glCoverageModulationNV;

#ifndef GL_RASTER_MULTISAMPLE_EXT
#define GL_RASTER_MULTISAMPLE_EXT 0x9327
#endif

int LoadMdlDataFromFile(const char* name, void** buffer)
{
    int32_t len;
    char* data = NvAssetLoaderRead(name, len);

    if (!data) return 0;
    *buffer = malloc(len+1);
    memcpy(*buffer, data, len);
    NvAssetLoaderFree(data);

    return len;
}

GLfloat* genLineSphere(int sphslices, int* bufferSize, float scale)
{
    GLfloat rho, drho, theta, dtheta;
    GLfloat x, y, z;
    GLfloat s, t, ds, dt;
    GLint i, j;
    GLfloat* buffer;

    int count=0;
    const int slices = (int)sphslices;
    const int stacks = slices;
    buffer = new GLfloat[(slices+1)*stacks*3*2];
    if (bufferSize) *bufferSize = (slices+1)*stacks*3*2*4;
    ds = GLfloat(1.0 / sphslices);//slices;
    dt = GLfloat(1.0 / sphslices);//stacks;
    t = 1.0;
    drho = GLfloat(NV_PI / (GLfloat) stacks);
    dtheta = GLfloat(2.0 * NV_PI / (GLfloat) slices);

    for (i= 0; i < stacks; i++) {
        rho = i * drho;

        s = 0.0;
        for (j = 0; j<slices; j++) { // having j<=slices was doubling up one line
            theta = (j==slices) ? GLfloat(0.0) : GLfloat(j * dtheta);
            x = -sin(theta) * sin(rho)*scale;
            z = cos(theta) * sin(rho)*scale;
            y = -cos(rho)*scale;
            buffer[count+0] = x;
            buffer[count+1] = y;
            buffer[count+2] = z;
            count+=3;

            x = -sin(theta) * sin(rho+drho)*scale;
            z = cos(theta) * sin(rho+drho)*scale;
            y = -cos(rho+drho)*scale;
            buffer[count+0] = x;
            buffer[count+1] = y;
            buffer[count+2] = z;
            count+=3;

            s += ds;
        }
        t -= dt;
    }
    return buffer;
}

BlendedAA::BlendedAA() :
    m_autoSpin(true), m_showFloor(true), m_AAType(0), m_AALevel(2), m_lineWidth(2.0), m_currentAAType(0), m_currentAALevel(0), m_memoryUsage(0)
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

BlendedAA::~BlendedAA()
{
    LOGI("BlendedAA: destroyed\n");
}

void BlendedAA::configurationCallback(NvGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionGL4();
}

void BlendedAA::initUI() {
    // sample apps automatically have a tweakbar they can use.
    if (mTweakBar) { // create our tweak ui
        mTweakBar->addValue("Auto Spin", m_autoSpin);
        mTweakBar->addValue("Show Floor", m_showFloor);
        mTweakBar->addPadding();
        mTweakBar->addValue("Line Width", m_lineWidth, 0.1f, 3.0f);

        mTweakBar->addPadding();

        NvTweakEnum<uint32_t> AAType[] = 
        {
            { "Off", 0 },
            { "MSAA", 1 },
            { "TIR", 2 },
        };
        mTweakBar->addEnum("AA Type", m_AAType, AAType, TWEAKENUM_ARRAYSIZE(AAType), 0x0);

        NvTweakEnum<uint32_t> AALevel[] =
        {
            { "2x", 2 },
            { "4x", 4 },
            { "8x", 8 },
        };
        mTweakBar->addEnum("AA Level:", m_AALevel, AALevel, TWEAKENUM_ARRAYSIZE(AALevel), 0x0);
        mTweakBar->addPadding();

    }
    if (mFPSText) {
        NvUIRect tr;
        mFPSText->GetScreenRect(tr); // base off of fps element.
        m_memoryUsage = new NvUIValueText("KB Used", NvUIFontFamily::SANS, (mFPSText->GetFontSize()*2)/3, NvUITextAlign::RIGHT, 0, NvUITextAlign::RIGHT);
        m_memoryUsage->SetColor(NV_PACKED_COLOR(0x30,0xD0,0xD0,0xB0));
        m_memoryUsage->SetShadow();
        mUIWindow->Add(m_memoryUsage, tr.left, tr.top+tr.height+8);
    }
}

bool BlendedAA::genRenderTexture()
{
    glGenFramebuffers(1, &m_sourceFrameBuffer);
    glGenRenderbuffers(1, &m_sourceDepthBuffer);

    int width = getGLContext()->width();
    int height = getGLContext()->height();
    glGenFramebuffers(1, &m_resolveFrameBuffer);
    glGenTextures(1, &m_resolveTexture);
    glBindTexture(GL_TEXTURE_2D, m_resolveTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, m_resolveFrameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_resolveTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());

    return updateRenderTexture();
}

bool BlendedAA::updateRenderTexture()
{
    const int width = getGLContext()->width();
    const int height = getGLContext()->height();

    glBindFramebuffer(GL_FRAMEBUFFER, m_sourceFrameBuffer);

    // deallocate the existing color buffer texture
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glDeleteTextures(1, &m_sourceTexture);
    glGenTextures(1, &m_sourceTexture);

    if (m_AAType == 1) {
        CHECK_GL_ERROR();
        // MSAA path, we have multisampled color buffer
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_sourceTexture);
        CHECK_GL_ERROR();
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_AALevel, GL_RGBA, width, height, true);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_sourceTexture, 0);
    } else {
        // No AA or TIR path, we have single sampled color buffer
        glBindTexture(GL_TEXTURE_2D, m_sourceTexture);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_sourceTexture, 0);
    }
    CHECK_GL_ERROR();

    glBindRenderbuffer(GL_RENDERBUFFER, m_sourceDepthBuffer);
    if (m_AAType == 0) {
        // No AA path, single sampled depth buffer
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    } else {
        // MSAA or TIR path, multisampled depth buffer
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_AALevel, GL_DEPTH_COMPONENT24, width, height);
    }
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_sourceDepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Finalize
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
    CHECK_GL_ERROR();
    return true;
}

void BlendedAA::initRendering(void) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);    

    NvAssetLoaderAddSearchPath("gl4-maxwell/BlendedAA");

    if(!requireMinAPIVersion(NvGLAPIVersionGL4_3())) return;

    if (!requireExtension("GL_EXT_raster_multisample")) return;

    m_hasGPUMemInfo = requireExtension("GL_NVX_gpu_memory_info", false);

    m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -3.0f));
    m_transformer->setRotationVec(nv::vec3f(NV_PI*0.15f, 0.0f, 0.0f));

    const float* buf = genLineSphere(40, &m_sphereBufferSize, 1);
    glGenBuffers(1, &m_sphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, m_sphereBufferSize, buf, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    delete[] buf;

    // make a quad for the floor
    const float quad[12] = {	0.0f, -2.0f, 0.0f,
        1.0f, -2.0f, 0.0f,
        1.0f, -2.0f, -1.0f,
        0.0f, -2.0f, -1.0f };
    glGenBuffers(1, &m_floorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_floorVBO);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), quad, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    int i;
    void* pData=0;
    for (i=0;i<2;i++) {
        m_objectFileSize[i] = LoadMdlDataFromFile(model_files[i], &pData);
        glGenBuffers(1, &m_object[i]);
        glBindBuffer(GL_ARRAY_BUFFER, m_object[i]);
        glBufferData(GL_ARRAY_BUFFER, m_objectFileSize[i], pData, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        free(pData);
    }

    m_LineProg = NvGLSLProgram::createFromFiles("shaders/DrawLine.vert", "shaders/DrawLine.frag");
    m_ObjectProg = NvGLSLProgram::createFromFiles("shaders/Object.vert", "shaders/Object.frag");
    m_FloorProg = NvGLSLProgram::createFromFiles("shaders/Floor.vert", "shaders/Floor.frag");
    m_TriProg = NvGLSLProgram::createFromFiles("shaders/Tris.vert", "shaders/Tris.frag");
    m_FanProg = NvGLSLProgram::createFromFiles("shaders/Ftransform.vert", "shaders/White.frag");

    genRenderTexture();

    nv::perspective(m_projection_matrix, FOV * 0.5f, getGLContext()->width()/(float)getGLContext()->height(), Z_NEAR, Z_FAR);

    // load additional functions
    glRasterSamplesEXT = (PFNGLRASTERSAMPLESEXTPROC) getGLContext()->getGLProcAddress("glRasterSamplesEXT");
    glCoverageModulationNV = (PFNGLCOVERAGEMODULATIONVPROC) getGLContext()->getGLProcAddress("glCoverageModulationNV");

    CHECK_GL_ERROR();
}

void BlendedAA::reshape(int32_t width, int32_t height)
{
    glViewport( 0, 0, (GLint) width, (GLint) height );
    updateRenderTexture();
    CHECK_GL_ERROR();
}

void BlendedAA::drawObjects(nv::matrix4f* mvp, const float* color, const int id)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_FRONT);

    glUseProgram(m_ObjectProg->getProgram());
    m_ObjectProg->setUniformMatrix4fv("mvp", mvp->_array);
    m_ObjectProg->setUniform4fv("color", color, 1);

    glBindBuffer(GL_ARRAY_BUFFER, m_object[id]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    NV_ASSERT(m_ObjectProg->getAttribLocation("aPosition") == 0);
    NV_ASSERT(m_ObjectProg->getAttribLocation("aNormal") == 1);
    glVertexAttribPointer(m_ObjectProg->getAttribLocation("aPosition"), 3, GL_FLOAT, GL_FALSE, 32, 0);
    glVertexAttribPointer(m_ObjectProg->getAttribLocation("aNormal"), 3, GL_FLOAT, GL_FALSE, 32, (void*)12);

    glDrawArrays(GL_TRIANGLES, 0, m_objectFileSize[id]/32);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void BlendedAA::drawSphere(nv::matrix4f* mvp)
{
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glUseProgram(m_LineProg->getProgram());
    m_LineProg->setUniformMatrix4fv("mvp",  mvp->_array);
    if (m_showFloor) {
        m_LineProg->setUniform4f("color", 0.0, 0.0, 1.0, 1.0);
    } else {
        m_LineProg->setUniform4f("color", 1.0, 1.0, 1.0, 1.0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_sphereVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_LINES, 0, 41 * 40 * 2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void BlendedAA::drawFloor(nv::matrix4f* mvp)
{
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    const static int instances = 5000;

    glUseProgram(m_FloorProg->getProgram());
    m_FloorProg->setUniformMatrix4fv("mvp", mvp->_array);
    m_FloorProg->setUniform1i("instances", instances);

    glBindBuffer(GL_ARRAY_BUFFER, m_floorVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glDrawArraysInstanced(GL_QUADS, 0, 4, instances);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(0);
}

void BlendedAA::drawAll(void)
{
    nv::matrix4f rotation;
    nv::matrix4f model;
    nv::matrix4f mvp;
    const float color0[4]={0.5,0.5,0.8,1.0};
    const float color1[4]={0.5,0.9,0.5,1.0};

    m_transformer->setRotationVel(nv::vec3f(0.0f, m_autoSpin ? (NV_PI*0.05f) : 0.0f, 0.0f));

    //draw lined sphere
    nv::matrix4f vp = m_projection_matrix * m_transformer->getModelViewMat();
    nv::rotationX(rotation, float(0.2));
    mvp = m_projection_matrix * m_transformer->getModelViewMat() * rotation;
    drawSphere(&mvp);

    //draw rings
    model.make_identity();
    model.set_scale(0.04);
    nv::rotationX(rotation, float(0.1));
    mvp = m_projection_matrix * m_transformer->getModelViewMat() * rotation* model;
    drawObjects(&mvp, color0, 0);

    model.make_identity();
    model.set_scale(0.04);
    nv::rotationX(rotation, float(NV_PI/2+0.1));
    mvp = m_projection_matrix * m_transformer->getModelViewMat() * rotation * model;
    drawObjects(&mvp, color0, 0);

    //draw tetrahedron
    model.make_identity();
    model.set_scale(0.03);
    model.set_translate(nv::vec3f(0.0f, -0.3f, 0.0f));
    mvp = m_projection_matrix * m_transformer->getModelViewMat() * model;
    drawObjects(&mvp, color1, 1);

    if (m_showFloor) {
        // draw floor
        mvp = m_projection_matrix * m_transformer->getModelViewMat();
        drawFloor(&mvp);
    }

    return;
}

void BlendedAA::draw(void)
{
    // if AA settings have changed, reallocate the framebuffer as necessary
    if (m_AALevel != m_currentAALevel || m_AAType != m_currentAAType) {
        updateRenderTexture();
        m_currentAAType = m_AAType;
        m_currentAALevel = m_AALevel;
    }

    // NOTE: additive blending depends on the starting color being unsaturated
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    if (m_AAType == 2) {
        // TIR on
        glRasterSamplesEXT(m_AALevel, true);
        glEnable(GL_RASTER_MULTISAMPLE_EXT);
    } else {
        glDisable(GL_RASTER_MULTISAMPLE_EXT);
    }
    if (m_AAType != 0) {
        // MSAA or TIR on
        glEnable(GL_MULTISAMPLE);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_sourceFrameBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLineWidth(m_lineWidth);

    if (m_AAType == 2) {
        // TIR path does a two pass method with z prepass
        // do a z prepass
        glDisable(GL_BLEND);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthFunc(GL_LESS);
        drawAll();
        // now do render pass
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        // set up additive blending
        glEnable(GL_BLEND);
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
        glCoverageModulationNV(GL_RGBA); // make TIR modulation scale all components by fractional coverage
    } else {
        glDisable(GL_BLEND);
    }
    drawAll();

    // reset masks so clear works properly
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());

    // resolve the color buffer
    const int width = getGLContext()->width();
    const int height = getGLContext()->height();
    if (m_AAType == 1) {
        // MSAA should resolve to a normal-size buffer before blitting to default framebuffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_sourceFrameBuffer);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolveFrameBuffer);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_resolveFrameBuffer);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, getMainFBO());
        glDrawBuffer(GL_BACK); // assuming double buffered default framebuffer
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, getMainFBO());
    } else {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_sourceFrameBuffer);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, getMainFBO());
        glDrawBuffer(GL_BACK); // assuming double buffered default framebuffer
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, getMainFBO());
    }

    // calculate GPU memory usage
    GLint totalmem = 0;
    GLint available = 0;
    if (m_hasGPUMemInfo) {
        glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalmem);
        glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &available);
    }
    m_memoryUsage->SetValue(static_cast<uint32_t>(totalmem - available));

    // for UI drawing, disable TIR mode
    if (m_AAType == 2)
        glDisable(GL_RASTER_MULTISAMPLE_EXT);
    return;
}

NvAppBase* NvAppFactory() {
    return new BlendedAA();
}
