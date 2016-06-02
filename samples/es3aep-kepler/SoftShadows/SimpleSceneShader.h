//----------------------------------------------------------------------------------
// File:        es3aep-kepler\SoftShadows/SimpleSceneShader.h
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
#ifndef SIMPLE_SCENE_SHADER_H
#define SIMPLE_SCENE_SHADER_H

#include "SceneShader.h"

class SimpleSceneShader
    : public SceneShader
{
public:

    SimpleSceneShader(
        const char *vertFilename = "shaders/debugrender.vert",
        const char *fragFilename = "shaders/debugrender.frag")
        : SceneShader(vertFilename, fragFilename)
    {
        // Uniforms
        m_useTextureHandle = getUniformLocation("g_useTexture");
        m_podiumCenterLocationHandle = getUniformLocation("g_podiumCenterWorld");
        m_groundDiffuseHandle = getUniformLocation("g_groundDiffuse");
        m_rockDiffuseHandle = getUniformLocation("g_rockDiffuse");		
    }

    virtual void setUseTexture(GLint tex) { setUniformInt(m_useTextureHandle, tex); }
    void setPodiumCenterLocation(const nv::vec3f &v) { setUniformVector(m_podiumCenterLocationHandle, v); }
    void setGroundDiffuseTexture(GLint unit) { setUniformTexture(m_groundDiffuseHandle, unit); }
    void setRockDiffuseTexture(GLint unit) { setUniformTexture(m_rockDiffuseHandle, unit); }

private:

    GLint m_useTextureHandle;
    GLint m_podiumCenterLocationHandle;
    GLint m_groundDiffuseHandle;
    GLint m_rockDiffuseHandle;	
};

#endif