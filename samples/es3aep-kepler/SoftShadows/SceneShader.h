//----------------------------------------------------------------------------------
// File:        es3aep-kepler\SoftShadows/SceneShader.h
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
#ifndef SCENE_SHADER_H
#define SCENE_SHADER_H

#include "Shader.h"
#include "RigidMesh.h"

class SceneShader
    : public Shader
    , public RigidMesh::IShader
{
public:

    SceneShader(
        const char *vertFilename,
        const char *fragFilename,
        const char *positionAttrName = "g_vPosition",
        const char *worldMatrixName = "g_world",
        const char *viewProjMatrixName = "g_viewProj")
        : Shader(vertFilename, fragFilename)
    {
        // Vertex attributes
        m_positionAttrHandle = getAttribLocation(positionAttrName);

        // Uniforms
        m_worldMatrixHandle = getUniformLocation(worldMatrixName);
        m_viewProjMatrixHandle = getUniformLocation(viewProjMatrixName);		
    }
    virtual ~SceneShader() {}

    virtual GLint getPositionAttrHandle() const { return m_positionAttrHandle; }
    virtual GLint getNormalAttrHandle() const { return -1; }

    void setWorldMatrix(const nv::matrix4f &m) { setUniformMatrix(m_worldMatrixHandle, m); }
    void setViewProjMatrix(const nv::matrix4f &m) { setUniformMatrix(m_viewProjMatrixHandle, m); }

    virtual void setUseDiffuse(bool useDiffuse) { }
    virtual void setUseTexture(GLint useTexture) { }

protected:

    // Vertex attributes
    GLint m_positionAttrHandle;

    // Uniforms
    GLint m_worldMatrixHandle;
    GLint m_viewProjMatrixHandle;	
};

#endif
