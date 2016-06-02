//----------------------------------------------------------------------------------
// File:        gl4-maxwell\CubemapRendering/CubemapRendering.h
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
#pragma once

#include <NvAppBase/gl/NvSampleAppGL.h>
#include <NV/NvMath.h>
#include <NvGLUtils/NvModelGL.h>

#define SAFE_DELETE(x) if(x) { delete x; x = nullptr; }
#define DEG2RAD(x) (x)*(3.141592f/180.0f)

class NvGLSLProgram;

struct CubemapMethod {
    enum Enum {
        FreezeCubemapRender,
        NoGs,
        NoGsCull,
        GsNoCull,
        GsCull,
        InstancedGsNoCull,
        InstancedGsCull,
        FgsCull,
        FgsNoCull,
        End
    };
};

struct ModelInstanceInfo {
    nv::vec3f translation;
    nv::quaternionf orientation;
    nv::vec3f color;
    nv::matrix4f modelMatrix;
};

class CubemapRendering : public NvSampleAppGL {
public:
    CubemapRendering();
    virtual ~CubemapRendering();

    virtual void configurationCallback(NvGLConfiguration& config);
    virtual void initRendering(void);
    virtual void initUI(void);
    virtual NvUIEventResponse handleReaction(const NvUIReaction &react);
    virtual void draw(void);
    virtual void reshape(int32_t width, int32_t height);
    virtual void shutdownRendering(void);
private:
    NvGLSLProgram* m_cubemapRenderNoGsProgram;
    NvGLSLProgram* m_cubemapRenderGsCullProgram;
    NvGLSLProgram* m_cubemapRenderGsNoCullProgram;
    NvGLSLProgram* m_cubemapRenderInstancedGsCullProgram;
    NvGLSLProgram* m_cubemapRenderInstancedGsNoCullProgram;
    NvGLSLProgram* m_cubemapRenderFgsCullProgram;
    NvGLSLProgram* m_cubemapRenderFgsNoCullProgram;
    NvGLSLProgram* m_cameraProgram;
    NvGLSLProgram* m_cameraReflectiveProgram;
    CubemapMethod::Enum m_method;
    nv::matrix4f m_projectionMatrix;
    NvModelGL* m_dragonModel;
    NvModelGL* m_sphereModel;

    int m_cubemapSize;
    GLuint m_cubemapColorTex;
    GLuint m_cubemapDepthTex;
    GLuint m_cubemapFramebuffer;

    uint32_t m_numDragons;
    std::vector<ModelInstanceInfo> m_dragons;

    bool m_enableAnimation;

    void createCubemap(int size);
    void destroyCubemap();
    void renderCubemap();
    void createDragons();
    void updateDragons(float dtime);
};

NvAppBase* NvAppFactory() {
    return new CubemapRendering();
}
