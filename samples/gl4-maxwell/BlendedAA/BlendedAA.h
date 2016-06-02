//----------------------------------------------------------------------------------
// File:        gl4-maxwell\BlendedAA/BlendedAA.h
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
#ifndef BlendedAA_H
#define BlendedAA_H

#include "NvAppBase/gl/NvSampleAppGL.h"
#include "NV/NvMath.h"

class NvStopWatch;
class NvFramerateCounter;
class NvGLSLProgram;

class BlendedAA : public NvSampleAppGL
{
public:
    BlendedAA();
    ~BlendedAA();

    void initUI(void);
    void initRendering(void);
    void draw(void);
    void drawObjects(nv::matrix4f* mvp, const float* color, const int id);
    void drawSphere(nv::matrix4f* mvp);
    void drawFloor(nv::matrix4f* mvp);
    void drawAll(void);
    void reshape(int32_t width, int32_t height);
    bool genRenderTexture();

    void configurationCallback(NvGLConfiguration& config);

private:
    bool updateRenderTexture();

protected:
    // TweakBar Options
    bool mOption;
    bool m_autoSpin;
    bool m_showFloor;
    float m_lineWidth;
    uint32_t m_AAType;
    uint32_t m_AALevel;
    NvUIValueText *m_memoryUsage;

    // currently enabled AA options
    uint32_t m_currentAAType;
    uint32_t m_currentAALevel;

    // buffers for drawable objects
    GLuint m_object[2];
    int m_objectFileSize[2];
    GLuint m_sphereVBO;
    int	m_sphereBufferSize;
    GLuint m_floorVBO;

    // texutres for framebuffers
    GLuint m_sourceTexture;
    GLuint m_sourceFrameBuffer;
    GLuint m_sourceDepthBuffer;

    GLuint m_resolveTexture;
    GLuint m_resolveFrameBuffer;

    bool m_hasGPUMemInfo;

    // shader programs
    NvGLSLProgram* m_ObjectProg;
    NvGLSLProgram* m_LineProg;
    NvGLSLProgram* m_FloorProg;
    NvGLSLProgram* m_FanProg;
    NvGLSLProgram* m_TriProg;
    nv::matrix4f m_projection_matrix;
};

#endif
