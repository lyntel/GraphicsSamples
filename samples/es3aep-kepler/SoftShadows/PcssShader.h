//----------------------------------------------------------------------------------
// File:        es3aep-kepler\SoftShadows/PcssShader.h
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
#ifndef PCSS_SHADER_H
#define PCSS_SHADER_H

#include "EyeShader.h"

class PcssShader
    : public EyeShader
{
public:

    PcssShader()
        : EyeShader("shaders/pcss.frag")
    {
        m_shadowTechniqueHandle = getUniformLocation("g_shadowTechnique");
        m_samplePatternHandle = getUniformLocation("g_samplePattern");
        m_lightViewMatrixHandle = getUniformLocation("g_lightView");
        m_lightRadiusUvHandle = getUniformLocation("g_lightRadiusUV");
        m_lightZNearHandle = getUniformLocation("g_lightZNear");
        m_lightZFarHandle = getUniformLocation("g_lightZFar");
    }

    void setShadowTechnique(GLint technique) { setUniformInt(m_shadowTechniqueHandle, technique); }
    void setSamplePattern(GLint preset) { setUniformInt(m_samplePatternHandle, preset); }
    void setLightViewMatrix(const nv::matrix4f &m) { setUniformMatrix(m_lightViewMatrixHandle, m); }
    void setLightRadiusUv(const nv::vec2f &v) { setUniformVector(m_lightRadiusUvHandle, v); }
    void setLightZNear(GLfloat zNear) { setUniformScalar(m_lightZNearHandle, zNear); }
    void setLightZFar(GLfloat zFar) { setUniformScalar(m_lightZFarHandle, zFar); }

private:

    GLint m_shadowTechniqueHandle;
    GLint m_samplePatternHandle;
    GLint m_lightViewMatrixHandle;
    GLint m_lightRadiusUvHandle;
    GLint m_lightZNearHandle;
    GLint m_lightZFarHandle;
};

#endif