//----------------------------------------------------------------------------------
// File:        es3aep-kepler\SoftShadows/EyeShader.h
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
#ifndef EYE_SHADER_H
#define EYE_SHADER_H

#include "SimpleSceneShader.h"

class EyeShader
    : public SimpleSceneShader
{
public:

    EyeShader(const char* fragFilename)
        : SimpleSceneShader("shaders/eyerender.vert", fragFilename)
    {
        // Vertex attributes
        m_normalAttrHandle = getAttribLocation("g_vNormal");

        // Uniforms        
        m_lightViewProjClip2TexMatrixHandle = getUniformLocation("g_lightViewProjClip2Tex");
        m_lightPosHandle = getUniformLocation("g_lightPos");
        m_groundNormalHandle = getUniformLocation("g_groundNormal");
        m_useDiffuseHandle = getUniformLocation("g_useDiffuse");
        m_shadowMapDepthHandle = getUniformLocation("g_shadowMapDepth");
        m_shadowMapPcfHandle = getUniformLocation("g_shadowMapPcf");
    }
    virtual ~EyeShader() {}

    virtual GLint getNormalAttrHandle() const { return m_normalAttrHandle; }

    void setLightViewProjClip2TexMatrix(const nv::matrix4f &m) { setUniformMatrix(m_lightViewProjClip2TexMatrixHandle, m); }
    void setLightPosition(const nv::vec3f &v) { setUniformVector(m_lightPosHandle, v); }
    void setGroundNormalTexture(GLint unit) { setUniformTexture(m_groundNormalHandle, unit); }
    virtual void setUseDiffuse(bool useDiffuse) { setUniformBool(m_useDiffuseHandle, useDiffuse); }
    void setShadowMapTexture(GLint depthUnit, GLint pcfUnit)
    {
        setUniformTexture(m_shadowMapDepthHandle, depthUnit);
        setUniformTexture(m_shadowMapPcfHandle, pcfUnit);
    }

private:

    GLint m_normalAttrHandle;

    GLint m_lightViewProjClip2TexMatrixHandle;
    GLint m_lightPosHandle;
    GLint m_groundNormalHandle;
    GLint m_useDiffuseHandle;
    GLint m_shadowMapDepthHandle;
    GLint m_shadowMapPcfHandle;
};

#endif
