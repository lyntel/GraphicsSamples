//----------------------------------------------------------------------------------
// File:        es2-aurora\SkinningApp/SkinningApp.h
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

#ifndef SKINNING_APP_H
#define SKINNING_APP_H

#include "NvAppBase/gl/NvSampleAppGL.h"

#include "KHR/khrplatform.h"
#include "NvGamepad/NvGamepad.h"
#include "NV/NvMath.h"
#include "NvAppBase/NvInputTransformer.h"
#include "SkinnedMesh.h"

#define TO_RADIANS 0.01745329f

class NvStopWatch;
class NvFramerateCounter;
class NvGLSLProgram;
class NvTweakBar;

class SkinningApp : public NvSampleAppGL
{
public:
    SkinningApp();
    ~SkinningApp();
    
    virtual void initRendering(void);
    virtual void initUI(void);
    virtual void draw(void);
    virtual void reshape(int32_t width, int32_t height);

    virtual void configurationCallback(NvGLConfiguration& config);

private:
    void computeBones(float t, float* bones);
    void copyMatrixToArray(nv::matrix4f& M, float* dest);

    SkinnedMesh      m_mesh;
    NvGLSLProgram*   m_skinningProgram;

    nv::matrix4f     m_MVP;

    bool             m_singleBoneSkinning;
    bool             m_pauseTime;
    bool             m_pausedByPerfHUD;

    float            m_time;
    float            m_timeScalar;

    uint32_t         m_renderMode;        // 0: Render Color   1: Render Normals    2: Render Weights

    // Uniform locations for uniforms in m_skinningProgram
    int32_t          m_ModelViewProjectionLocation;
    int32_t          m_BonesLocation;
    int32_t          m_RenderModeLocation;
    int32_t          m_LightDir0Location;
    int32_t          m_LightDir1Location;

    // Attribute locations for m_skinningProgram
    int32_t          m_iPositionLocation;
    int32_t          m_iNormalLocation;
    int32_t          m_iWeightsLocation;
};

#endif
