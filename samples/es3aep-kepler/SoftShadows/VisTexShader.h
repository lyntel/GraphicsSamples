//----------------------------------------------------------------------------------
// File:        es3aep-kepler\SoftShadows/VisTexShader.h
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
#ifndef VIS_TEX_SHADER_H
#define VIS_TEX_SHADER_H

#include <NvGLUtils/NvGLSLProgram.h>

class VisTexShader
    : public Shader
{
public:

    VisTexShader()
        : Shader("shaders/vistex.vert", "shaders/vistex.frag")
    {
        // Vertex attributes
        m_positionAttrHandle = getAttribLocation("g_vPosition");
        m_texCoordAttrHandle = getAttribLocation("g_vTexCoord");

        // Uniforms
        m_lightZNearHandle = getUniformLocation("g_lightZNear");
        m_lightZFarHandle = getUniformLocation("g_lightZFar");
        m_shadowMapHandle = getUniformLocation("g_shadowMap");
    }

    GLint getPositionAttrHandle() const { return m_positionAttrHandle; }
    GLint getTexCoordAttrHandle() const { return m_texCoordAttrHandle; }

    void setLightZNear(GLfloat zNear) { setUniformScalar(m_lightZNearHandle, zNear); }
    void setLightZFar(GLfloat zFar) { setUniformScalar(m_lightZFarHandle, zFar); }
    void setShadowMapTexture(GLint depthUnit) { setUniformTexture(m_shadowMapHandle, depthUnit);	}

private:

    GLint m_positionAttrHandle;
    GLint m_texCoordAttrHandle;

    GLint m_lightZNearHandle;
    GLint m_lightZFarHandle;
    GLint m_shadowMapHandle;
};

#endif