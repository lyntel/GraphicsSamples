//----------------------------------------------------------------------------------
// File:        gl4-maxwell\CubemapRendering/CubemapRendering.cpp
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
#include "CubemapRendering.h"
#include <NV/NvLogs.h>
#include <NvAppBase/NvFramerateCounter.h>
#include <NvAppBase/NvInputTransformer.h>
#include <NvModel/NvModel.h>
#include <NvUI/NvTweakBar.h>
#include <NvUI/NvTweakVar.h>
#include <NvGLUtils/NvGLSLProgram.h>
#include <NvAssetLoader/NvAssetLoader.h>
#include <KHR/khrplatform.h>

#define APP_TITLE "Cubemap Rendering"

struct UIReactionIDs {
    enum Enum
    {
        ObjectCount  = 0x10000001,
    };
};

#define GL_VIEWPORT_SWIZZLE_POSITIVE_X_NV                  0x9350
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_X_NV                  0x9351
#define GL_VIEWPORT_SWIZZLE_POSITIVE_Y_NV                  0x9352
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_Y_NV                  0x9353
#define GL_VIEWPORT_SWIZZLE_POSITIVE_Z_NV                  0x9354
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_Z_NV                  0x9355
#define GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV                  0x9356
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_W_NV                  0x9357

typedef void (KHRONOS_APIENTRY *PGNGLVIEWPORTSWIZZLENV)(GLuint index, GLenum swizzlex, GLenum swizzley, GLenum swizzlez, GLenum swizzlew);

PGNGLVIEWPORTSWIZZLENV glViewportSwizzleNV = nullptr;


CubemapRendering::CubemapRendering()
    : m_cubemapRenderNoGsProgram(nullptr)
    , m_cubemapRenderGsCullProgram(nullptr)
    , m_cubemapRenderGsNoCullProgram(nullptr)
    , m_cubemapRenderInstancedGsCullProgram(nullptr)
    , m_cubemapRenderInstancedGsNoCullProgram(nullptr)
    , m_cubemapRenderFgsCullProgram(nullptr)
    , m_cubemapRenderFgsNoCullProgram(nullptr)
    , m_cameraProgram(nullptr)
    , m_cameraReflectiveProgram(nullptr)
    , m_method(CubemapMethod::GsNoCull)
    , m_dragonModel(nullptr)
    , m_sphereModel(nullptr)
    , m_cubemapColorTex(0)
    , m_cubemapDepthTex(0)
    , m_cubemapFramebuffer(0)
    , m_cubemapSize(1024)
    , m_numDragons(6)
    , m_enableAnimation(true)
{
    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

CubemapRendering::~CubemapRendering() {
    SAFE_DELETE(m_cubemapRenderNoGsProgram);
    SAFE_DELETE(m_cubemapRenderGsCullProgram);
    SAFE_DELETE(m_cubemapRenderGsNoCullProgram);
    SAFE_DELETE(m_cubemapRenderInstancedGsCullProgram);
    SAFE_DELETE(m_cubemapRenderInstancedGsNoCullProgram);
    SAFE_DELETE(m_cubemapRenderFgsCullProgram);
    SAFE_DELETE(m_cubemapRenderFgsNoCullProgram);
    SAFE_DELETE(m_cameraProgram);
    SAFE_DELETE(m_cameraReflectiveProgram);
    SAFE_DELETE(m_dragonModel);
    SAFE_DELETE(m_sphereModel);
    destroyCubemap();
}

void CubemapRendering::configurationCallback(NvGLConfiguration& config) { 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGLAPIVersionGL4();
}

void CubemapRendering::initRendering(void) {
    if (!requireMinAPIVersion(NvGLAPIVersionGL4()))
        return;
    
    bool multiprojectionSupported = 
        requireExtension("GL_NV_viewport_array2", false) && 
        requireExtension("GL_NV_geometry_shader_passthrough", false);
    
    if(multiprojectionSupported)
    {
        glViewportSwizzleNV = (PGNGLVIEWPORTSWIZZLENV)getGLContext()->getGLProcAddress("glViewportSwizzleNV");
    }

    NvAssetLoaderAddSearchPath("gl4-maxwell/CubemapRendering");
    int32_t len;

    m_cameraProgram = NvGLSLProgram::createFromFiles("shaders/Camera.vert", "shaders/Camera.frag");
    m_cameraReflectiveProgram = NvGLSLProgram::createFromFiles("shaders/Camera.vert", "shaders/CameraReflective.frag");

    NvGLSLProgram::setGlobalShaderHeader("#version 440 core \n");
    m_cubemapRenderNoGsProgram = NvGLSLProgram::createFromFiles("shaders/CubemapNoGs.vert", "shaders/Cubemap.frag");

    char* cubemapVertSrc = NvAssetLoaderRead("shaders/Cubemap.vert", len);
    char* cubemapGsSrc = NvAssetLoaderRead("shaders/CubemapGs.geom", len);
    char* cubemapFgsSrc = NvAssetLoaderRead("shaders/CubemapFgs.geom", len);
    char* cubemapFragSrc = NvAssetLoaderRead("shaders/Cubemap.frag", len);

    NvGLSLProgram::ShaderSourceItem src[3];
    src[0].type = GL_VERTEX_SHADER;   src[0].src = cubemapVertSrc;
    src[1].type = GL_GEOMETRY_SHADER; src[1].src = cubemapGsSrc;
    src[2].type = GL_FRAGMENT_SHADER; src[2].src = cubemapFragSrc;

    NvGLSLProgram::setGlobalShaderHeader("#version 440 core \n #define USE_CULLING 1 \n #define USE_INSTANCING 0 \n");
    m_cubemapRenderGsCullProgram = new NvGLSLProgram();
    m_cubemapRenderGsCullProgram->setSourceFromStrings(src, 3);

    NvGLSLProgram::setGlobalShaderHeader("#version 440 core \n #define USE_CULLING 0 \n #define USE_INSTANCING 0 \n");
    m_cubemapRenderGsNoCullProgram = new NvGLSLProgram();
    m_cubemapRenderGsNoCullProgram->setSourceFromStrings(src, 3);

    NvGLSLProgram::setGlobalShaderHeader("#version 440 core \n #define USE_CULLING 1 \n #define USE_INSTANCING 1 \n");
    m_cubemapRenderInstancedGsCullProgram = new NvGLSLProgram();
    m_cubemapRenderInstancedGsCullProgram->setSourceFromStrings(src, 3);

    NvGLSLProgram::setGlobalShaderHeader("#version 440 core \n #define USE_CULLING 0 \n #define USE_INSTANCING 1 \n");
    m_cubemapRenderInstancedGsNoCullProgram = new NvGLSLProgram();
    m_cubemapRenderInstancedGsNoCullProgram->setSourceFromStrings(src, 3);
    
    if(multiprojectionSupported)
    {
        src[1].type = GL_GEOMETRY_SHADER; src[1].src = cubemapFgsSrc;

        NvGLSLProgram::setGlobalShaderHeader("#version 440 core \n #define USE_CULLING 1 \n");
        m_cubemapRenderFgsCullProgram = new NvGLSLProgram();
        m_cubemapRenderFgsCullProgram->setSourceFromStrings(src, 3);

        NvGLSLProgram::setGlobalShaderHeader("#version 440 core \n #define USE_CULLING 0 \n");
        m_cubemapRenderFgsNoCullProgram = new NvGLSLProgram();
        m_cubemapRenderFgsNoCullProgram->setSourceFromStrings(src, 3);
    }
    else
    {
        showDialog(APP_TITLE, "Multi-projection acceleration is not available. Fast GS rendering methods will not work.", false);
    }

    NvGLSLProgram::setGlobalShaderHeader(nullptr);

    NvAssetLoaderFree(cubemapVertSrc);
    NvAssetLoaderFree(cubemapGsSrc);
    NvAssetLoaderFree(cubemapFgsSrc);
    NvAssetLoaderFree(cubemapFragSrc);


	char* dragonSrc = NvAssetLoaderRead("models/dragon.obj", len);
	m_dragonModel = NvModelGL::CreateFromObj(
		(uint8_t *)dragonSrc, 1.0f, true, false);
    NvAssetLoaderFree(dragonSrc);
    
    char* sphereSrc = NvAssetLoaderRead("models/sphere.obj", len);
	m_sphereModel = NvModelGL::CreateFromObj(
		(uint8_t *)sphereSrc, 1.0f, true, false);
    NvAssetLoaderFree(sphereSrc);

    m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -6.0f));
    m_transformer->setRotationVec(nv::vec3f(DEG2RAD(30.0f), DEG2RAD(30.0f), 0.0f));

    createCubemap(m_cubemapSize);

    createDragons();

    CHECK_GL_ERROR();
}

void CubemapRendering::createCubemap(int size) {
    glGenTextures(1, &m_cubemapColorTex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapColorTex);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    for(int face = 0; face < 6; face++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA8, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    CHECK_GL_ERROR();

    glGenTextures(1, &m_cubemapDepthTex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapDepthTex);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    for(int face = 0; face < 6; face++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_DEPTH_COMPONENT32F, size, size, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    CHECK_GL_ERROR();

    glGenFramebuffers(1, &m_cubemapFramebuffer);

    CHECK_GL_ERROR();
}

void CubemapRendering::destroyCubemap() {
    glDeleteTextures(1, &m_cubemapColorTex);
    m_cubemapColorTex = 0;

    glDeleteTextures(1, &m_cubemapDepthTex);
    m_cubemapDepthTex = 0;

    glDeleteFramebuffers(1, &m_cubemapFramebuffer);
    m_cubemapFramebuffer = 0;
}

void CubemapRendering::initUI(void) {
    if (mTweakBar)
    {
        NvTweakEnum<uint32_t> cubemapMethods[] =
        {
            { "Freeze Cubemap Render", CubemapMethod::FreezeCubemapRender },
            { "VS-only without Culling", CubemapMethod::NoGs },
            { "VS-only with Per-Mesh Culling", CubemapMethod::NoGsCull },
            { "Normal GS without Culling", CubemapMethod::GsNoCull },
            { "Normal GS with Culling", CubemapMethod::GsCull },
            { "Instanced GS without Culling", CubemapMethod::InstancedGsNoCull },
            { "Instanced GS with Culling", CubemapMethod::InstancedGsCull },
            { "Fast GS without Culling", CubemapMethod::FgsNoCull },
            { "Fast GS with Culling", CubemapMethod::FgsCull }
        };
        mTweakBar->addEnum("Cubemap Rendering Method", (uint32_t&)m_method, cubemapMethods, TWEAKENUM_ARRAYSIZE(cubemapMethods));
        mTweakBar->addValue("Number of Dynamic Objects", m_numDragons, 1, 10, 1, UIReactionIDs::ObjectCount);
        mTweakBar->addValue("Enable Animation", m_enableAnimation);
    }

    mFramerate->setMaxReportRate(0.25f);
    mFramerate->setReportFrames(5);
    getAppContext()->setSwapInterval(0);
    CHECK_GL_ERROR();
}

NvUIEventResponse CubemapRendering::handleReaction(const NvUIReaction &react) {
    switch (react.code) {
    case UIReactionIDs::ObjectCount:
        createDragons();
        break;
    }

    return nvuiEventHandled;
}

nv::vec4f getSwizzleVector(GLenum src) {
    nv::vec4f v;

    switch(src) {
    case GL_VIEWPORT_SWIZZLE_POSITIVE_X_NV: v.x =  1; break;
    case GL_VIEWPORT_SWIZZLE_NEGATIVE_X_NV: v.x = -1; break;
    case GL_VIEWPORT_SWIZZLE_POSITIVE_Y_NV: v.y =  1; break;
    case GL_VIEWPORT_SWIZZLE_NEGATIVE_Y_NV: v.y = -1; break;
    case GL_VIEWPORT_SWIZZLE_POSITIVE_Z_NV: v.z =  1; break;
    case GL_VIEWPORT_SWIZZLE_NEGATIVE_Z_NV: v.z = -1; break;
    case GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV: v.w =  1; break;
    case GL_VIEWPORT_SWIZZLE_NEGATIVE_W_NV: v.w = -1; break;
    }

    return v;
}

nv::matrix4f createSwizzleProjectionMatrix(GLenum x, GLenum y, GLenum z, GLenum w) {
    nv::matrix4f m;
    m.set_row(0, getSwizzleVector(x));
    m.set_row(1, getSwizzleVector(y));
    m.set_row(2, getSwizzleVector(z));
    m.set_row(3, getSwizzleVector(w));
    return m;
}

#if 0
const GLenum SwizzlePatterns[6][4] = {
        // face X positive
    {   GL_VIEWPORT_SWIZZLE_POSITIVE_Z_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_Y_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_X_NV },

        // face X negative
    {   GL_VIEWPORT_SWIZZLE_NEGATIVE_Z_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_Y_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV, 
        GL_VIEWPORT_SWIZZLE_NEGATIVE_X_NV }, 

        // face Y positive
    {   GL_VIEWPORT_SWIZZLE_NEGATIVE_X_NV, 
        GL_VIEWPORT_SWIZZLE_NEGATIVE_Z_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_Y_NV },

        // face Y negative
    {   GL_VIEWPORT_SWIZZLE_NEGATIVE_X_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_Z_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV, 
        GL_VIEWPORT_SWIZZLE_NEGATIVE_Y_NV },

        // face Z positive
    {   GL_VIEWPORT_SWIZZLE_NEGATIVE_X_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_Y_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_Z_NV },

        // face Z negative
    {   GL_VIEWPORT_SWIZZLE_POSITIVE_X_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_Y_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV, 
        GL_VIEWPORT_SWIZZLE_NEGATIVE_Z_NV }
};
#else
const GLenum SwizzlePatterns[6][4] = {
        // face X positive
    {   GL_VIEWPORT_SWIZZLE_NEGATIVE_Z_NV, 
        GL_VIEWPORT_SWIZZLE_NEGATIVE_Y_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_X_NV },

        // face X negative
    {   GL_VIEWPORT_SWIZZLE_POSITIVE_Z_NV, 
        GL_VIEWPORT_SWIZZLE_NEGATIVE_Y_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV, 
        GL_VIEWPORT_SWIZZLE_NEGATIVE_X_NV }, 

        // face Y positive
    {   GL_VIEWPORT_SWIZZLE_POSITIVE_X_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_Z_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_Y_NV },

        // face Y negative
    {   GL_VIEWPORT_SWIZZLE_POSITIVE_X_NV, 
        GL_VIEWPORT_SWIZZLE_NEGATIVE_Z_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV, 
        GL_VIEWPORT_SWIZZLE_NEGATIVE_Y_NV },

        // face Z positive
    {   GL_VIEWPORT_SWIZZLE_POSITIVE_X_NV, 
        GL_VIEWPORT_SWIZZLE_NEGATIVE_Y_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_Z_NV },

        // face Z negative
    {   GL_VIEWPORT_SWIZZLE_NEGATIVE_X_NV, 
        GL_VIEWPORT_SWIZZLE_NEGATIVE_Y_NV, 
        GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV, 
        GL_VIEWPORT_SWIZZLE_NEGATIVE_Z_NV }
};
#endif

void CubemapRendering::renderCubemap() {
    NvGLSLProgram* program = nullptr;
    bool useViewportSwizzle = false;
    bool useGS = true;

    switch(m_method)
    {
    case CubemapMethod::NoGs:
    case CubemapMethod::NoGsCull:
        program = m_cubemapRenderNoGsProgram;
        useGS = false;
        break;
    case CubemapMethod::GsCull:
        program = m_cubemapRenderGsCullProgram;
        break;
    case CubemapMethod::GsNoCull:
        program = m_cubemapRenderGsNoCullProgram;
        break;
    case CubemapMethod::InstancedGsCull:
        program = m_cubemapRenderInstancedGsCullProgram;
        break;
    case CubemapMethod::InstancedGsNoCull:
        program = m_cubemapRenderInstancedGsNoCullProgram;
        break;
    case CubemapMethod::FgsCull:
        program = m_cubemapRenderFgsCullProgram;
        useViewportSwizzle = true;
        break;
    case CubemapMethod::FgsNoCull:
        program = m_cubemapRenderFgsNoCullProgram;
        useViewportSwizzle = true;
        break;
    }

    if(!program)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, m_cubemapFramebuffer);
    
    glClearColor( 0.1f, 0.1f, 0.1f, 1.0f);
    glEnable( GL_DEPTH_TEST );
    glClearDepth( 0.0f );       // 1/w depth buffer: depth = 0 means infinity
    glDepthFunc( GL_GREATER );  // 1/w depth buffer: closer objects have greater depth

    if(useViewportSwizzle) {
        for(int face = 0; face < 6; face++) {
            glViewportIndexedf(face, 0, 0, (float)m_cubemapSize, (float)m_cubemapSize);
            glViewportSwizzleNV(face, 
                SwizzlePatterns[face][0], 
                SwizzlePatterns[face][1], 
                SwizzlePatterns[face][2], 
                SwizzlePatterns[face][3]);
        }
    }
    else {
        glViewport(0, 0, m_cubemapSize, m_cubemapSize);
    }
    
    program->enable();
    
    program->setUniform3f("g_lightDir", 0.58f, 0.58f, 0.58f);

    nv::matrix4f viewMatrix = nv::matrix4f();
    program->setUniformMatrix4fv("g_viewMatrix", viewMatrix._array);
    
    if(useGS) {
        // attach all 6 faces as a layered texture
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_cubemapColorTex, 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_cubemapDepthTex, 0);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for(uint32_t n = 0; n < m_numDragons; n++) {
            program->setUniform3fv("g_color", &m_dragons[n].color.x);
            program->setUniformMatrix4fv("g_modelMatrix", m_dragons[n].modelMatrix._array);
            m_dragonModel->drawElements(0, 1);
        }
    }
    else {
        for(int face = 0; face < 6; face++) {
            // attach one face
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, m_cubemapColorTex, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,m_cubemapDepthTex, 0);
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            nv::matrix4f projectionMatrix = createSwizzleProjectionMatrix(
                SwizzlePatterns[face][0], 
                SwizzlePatterns[face][1], 
                SwizzlePatterns[face][2], 
                SwizzlePatterns[face][3]);

            program->setUniformMatrix4fv("g_projMatrix", projectionMatrix._array);

            nv::vec3f minExt = m_dragonModel->GetMinExt();
            nv::vec3f maxExt = m_dragonModel->GetMaxExt();
            nv::vec3f bboxVertices[8];
            bboxVertices[0] = nv::vec3f(minExt.x, minExt.y, minExt.z);
            bboxVertices[1] = nv::vec3f(minExt.x, minExt.y, maxExt.z);
            bboxVertices[2] = nv::vec3f(minExt.x, maxExt.y, minExt.z);
            bboxVertices[3] = nv::vec3f(minExt.x, maxExt.y, maxExt.z);
            bboxVertices[4] = nv::vec3f(maxExt.x, minExt.y, minExt.z);
            bboxVertices[5] = nv::vec3f(maxExt.x, minExt.y, maxExt.z);
            bboxVertices[6] = nv::vec3f(maxExt.x, maxExt.y, minExt.z);
            bboxVertices[7] = nv::vec3f(maxExt.x, maxExt.y, maxExt.z);
            
            for(uint32_t n = 0; n < m_numDragons; n++) {
                nv::matrix4f mvp = projectionMatrix * m_dragons[n].modelMatrix;

                if(m_method == CubemapMethod::NoGsCull) {
                    // Start with "object is culled by all planes"
                    int cullPlanes = 0x1f;

                    // Iterate over bbox vertices, transform them into clip space.
                    for(int v = 0; v < 8; v++) {
                        nv::vec4f transformed = mvp * nv::vec4f(bboxVertices[v], 1);

                        // Test the vertex against all frustum planes in 4D clip space.
                        // If the vertex is NOT culled by some plane, clear the corresponding bit.
                        if(transformed.w >  transformed.x) cullPlanes &= ~0x01;
                        if(transformed.w > -transformed.x) cullPlanes &= ~0x02;
                        if(transformed.w >  transformed.y) cullPlanes &= ~0x04;
                        if(transformed.w > -transformed.y) cullPlanes &= ~0x08;
                        if(transformed.w >  transformed.z) cullPlanes &= ~0x10;
                     // No need in this one since we use 1/w projection, and Z is always 1.0
                     // if(transformed.w > -transformed.z) cullPlanes &= ~0x20;
                    }

                    // If there is a plane that culls ALL the vertices, its bit will be set.
                    // In that case, do not draw the object.
                    if(cullPlanes != 0) continue;
                }

                program->setUniform3fv("g_color", &m_dragons[n].color.x);
                program->setUniformMatrix4fv("g_modelMatrix", m_dragons[n].modelMatrix._array);
                m_dragonModel->drawElements(0, 1);
            }
        }
    }

    program->disable();

    glBindFramebuffer(GL_FRAMEBUFFER, getMainFBO());
    
    // reset the swizzle state
    if(useViewportSwizzle) {
        for(int face = 0; face < 6; face++) {
            glViewportSwizzleNV(0, 
                GL_VIEWPORT_SWIZZLE_POSITIVE_X_NV, 
                GL_VIEWPORT_SWIZZLE_POSITIVE_Y_NV, 
                GL_VIEWPORT_SWIZZLE_POSITIVE_Z_NV, 
                GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV);
        }
    }

    CHECK_GL_ERROR();
}

void CubemapRendering::draw(void) {
    if(m_enableAnimation) {
        updateDragons(getFrameDeltaTime());
    }

    renderCubemap();

    glViewport( 0, 0, m_width, m_height );

    glClearColor( 0.5f, 0.5f, 0.5f, 1.0f);
    glClearDepth( 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    nv::matrix4f viewMatrix = m_transformer->getModelViewMat();
    nv::matrix4f modelMatrix = nv::matrix4f();


    m_cameraReflectiveProgram->enable();

    m_cameraReflectiveProgram->setUniformMatrix4fv("g_viewMatrix", viewMatrix._array);
    m_cameraReflectiveProgram->setUniformMatrix4fv("g_projMatrix", m_projectionMatrix._array);
    m_cameraReflectiveProgram->setUniformMatrix4fv("g_modelMatrix", modelMatrix._array);

    nv::vec4f cameraPos = inverse(viewMatrix) * nv::vec4f(0, 0, 0, 1);
    m_cameraReflectiveProgram->setUniform3f("g_cameraPos", cameraPos.x, cameraPos.y, cameraPos.z);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapColorTex);

    m_sphereModel->drawElements(0, 1);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    m_cameraReflectiveProgram->disable();

    CHECK_GL_ERROR();


    m_cameraProgram->enable();

    m_cameraProgram->setUniformMatrix4fv("g_viewMatrix", viewMatrix._array);
    m_cameraProgram->setUniformMatrix4fv("g_projMatrix", m_projectionMatrix._array);
    m_cameraProgram->setUniform3f("g_lightDir", 0.58f, 0.58f, 0.58f);

    for(uint32_t n = 0; n < m_numDragons; n++) {
        m_cameraProgram->setUniform3fv("g_color", &m_dragons[n].color.x);
        m_cameraProgram->setUniformMatrix4fv("g_modelMatrix", m_dragons[n].modelMatrix._array);
        m_dragonModel->drawElements(0, 1);
    }

    m_cameraProgram->disable();

    CHECK_GL_ERROR();
}

void CubemapRendering::reshape(int32_t width, int32_t height) {
    nv::perspective(m_projectionMatrix, DEG2RAD(45.0f), (float)width/(float)height, 0.1f, 20.0f);

    CHECK_GL_ERROR();
}

void CubemapRendering::shutdownRendering(void) {
    CHECK_GL_ERROR();
}

void CubemapRendering::createDragons(void) {
    if(m_dragons.size() == m_numDragons)
        return;

    m_dragons.resize(m_numDragons);

    const float radius = 2.1f;

    m_dragons[0].color = nv::vec3f(1, 0.5f, 0.5f);
    m_dragons[0].translation = nv::vec3f(0, radius, 0);
    m_dragons[0].orientation = nv::quaternionf(nv::vec3f(0, 1, 0), 0);

    m_dragons[1].color = nv::vec3f(0.5f, 0.5f, 1);
    m_dragons[1].translation = nv::vec3f(0, -radius, 0);
    m_dragons[1].orientation = nv::quaternionf(nv::vec3f(0, 1, 0), 0);

    for(uint32_t n = 2; n < m_numDragons; n++) {
        ModelInstanceInfo& instance = m_dragons[n];
        
        instance.color = nv::vec3f(
            ((float)rand() / RAND_MAX) * 0.8f + 0.2f,
            ((float)rand() / RAND_MAX) * 0.8f + 0.2f,
            ((float)rand() / RAND_MAX) * 0.8f + 0.2f);

        float angle = (2 * 3.14159f * (n-2)) / (m_numDragons-2);
        instance.translation = nv::vec3f(sin(angle) * radius, 0, cos(angle) * radius);

        instance.orientation = nv::quaternionf(nv::vec3f(0, 1, 0), (float)n);
    }

    updateDragons(0);
}

void CubemapRendering::updateDragons(float dt) {
    for(uint32_t n = 0; n < m_numDragons; n++) {
        ModelInstanceInfo& instance = m_dragons[n];
        
        float dAngle = dt * 0.1f;
        instance.translation = nv::vec3f(
            instance.translation.x * cos(dAngle) - instance.translation.z * sin(dAngle),
            instance.translation.y, 
            instance.translation.x * sin(dAngle) + instance.translation.z * cos(dAngle)
        );

        instance.orientation *= nv::quaternionf(nv::vec3f(0, 1, 0), dAngle);

        nv::matrix4f rotationM, translationM;
        instance.orientation.get_value(rotationM);
        translationM.set_translate(instance.translation);

        instance.modelMatrix = translationM * rotationM;
    }
}