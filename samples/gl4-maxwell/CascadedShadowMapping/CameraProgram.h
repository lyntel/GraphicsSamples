//----------------------------------------------------------------------------------
// File:        gl4-maxwell\CascadedShadowMapping/CameraProgram.h
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

#include <NvGLUtils/NvGLSLProgram.h>
#include <NvAssetLoader/NvAssetLoader.h>
#include <NV/NvMath.h>

class CameraProgram : public NvGLSLProgram
{
public:

    CameraProgram()
        : NvGLSLProgram()
        , m_positionAttrHandle(-1)
        , m_normalAttrHandle(-1)
        , m_modelMatrixUHandle(-1)
        , m_viewMatrixUHandle(-1)
        , m_projMatrixUHandle(-1)
        , m_lightDirUHandle(-1)
        , m_lightVPSBMatricesUHandle(-1)
        , m_normalizedFarPlanesUHandle(-1)
        , m_ambientUHandle(-1)
        , m_diffuseAlbedoUHandle(-1)
        , m_specularAlbedoUHandle(-1)
        , m_specularPowerUHandle(-1)
    {

    }

    bool init(const char* vertexProgramPath,
        const char* fragmentProgramPath,
        const char* positionAttrName = "position",
        const char* normalAttrName = "normal",
        const char* modelMatrixName = "modelMatrix",
        const char* viewMatrixName = "viewMatrix",
        const char* projMatrixName = "projMatrix",
        const char* lightMatrixName = "lightMatrix",
        const char* lightDirName = "lightDir",
        const char* lightVPSBMatricesName = "lightVPSBMatrices",
        const char* normalizedFarPlanesName = "normalizedFarPlanes",
        const char* ambientName = "ambient",
        const char* diffuseAlbedoName = "diffuseAlbedo",
        const char* specularAlbedoName = "specularAlbedo",
        const char* specularPowerName = "specularPower")
    {
        int32_t len;
        char* vertSrc = NvAssetLoaderRead(vertexProgramPath, len);
        char* fragSrc = NvAssetLoaderRead(fragmentProgramPath, len);
        if (!vertSrc || !fragSrc) {
            NvAssetLoaderFree(vertSrc);
            NvAssetLoaderFree(fragSrc);
            return false;
        }

        ShaderSourceItem items[2];
        items[0].src = vertSrc;
        items[0].type = GL_VERTEX_SHADER;
        items[1].src = fragSrc;
        items[1].type = GL_FRAGMENT_SHADER;

        setSourceFromStrings(items, 2);

        NvAssetLoaderFree(vertSrc);
        NvAssetLoaderFree(fragSrc);

        // Vertex attributes
        m_positionAttrHandle = getAttribLocation(positionAttrName);
        m_normalAttrHandle = getAttribLocation(normalAttrName);

        // Uniforms
        m_modelMatrixUHandle = getUniformLocation(modelMatrixName);
        m_viewMatrixUHandle = getUniformLocation(viewMatrixName);
        m_projMatrixUHandle = getUniformLocation(projMatrixName);
        m_lightDirUHandle = getUniformLocation(lightDirName);
        m_lightVPSBMatricesUHandle = getUniformLocation(lightVPSBMatricesName);
        m_normalizedFarPlanesUHandle = getUniformLocation(normalizedFarPlanesName);
        m_ambientUHandle = getUniformLocation(ambientName);
        m_diffuseAlbedoUHandle = getUniformLocation(diffuseAlbedoName);
        m_specularAlbedoUHandle = getUniformLocation(specularAlbedoName);
        m_specularPowerUHandle = getUniformLocation(specularPowerName);

        return true;
    }

    GLint getPositionAttrHandle() const { return m_positionAttrHandle; }
    GLint getNormalAttrHandle() const { return m_normalAttrHandle; }

    void setModelMatrix(const nv::matrix4f& modelMatrix) { setUniformMatrix4fv(m_modelMatrixUHandle, const_cast<GLfloat*>(modelMatrix.get_value())); }
    void setViewMatrix(const nv::matrix4f& viewMatrix) { setUniformMatrix4fv(m_viewMatrixUHandle, const_cast<GLfloat*>(viewMatrix.get_value())); }
    void setProjMatrix(const nv::matrix4f& projMatrix) { setUniformMatrix4fv(m_projMatrixUHandle, const_cast<GLfloat*>(projMatrix.get_value())); }
    void setLightDir(const nv::vec3f& lightDir) { setUniform3fv(m_lightDirUHandle, lightDir.get_value()); }
    void setLightVPSBMatrices(const nv::matrix4f lightVPSBMatrices[], int32_t count) { setUniformMatrix4fv(m_lightVPSBMatricesUHandle, const_cast<GLfloat*>(lightVPSBMatrices[0].get_value()), count); }
    void setNormalizedFarPlanes(const nv::vec4f& normalizedFarPlanes) { setUniform4fv(m_normalizedFarPlanesUHandle, normalizedFarPlanes.get_value()); }
    void setAmbient(const nv::vec3f& ambient) { setUniform3fv(m_ambientUHandle, ambient.get_value()); }
    void setDiffuseAlbedo(const nv::vec3f& diffuseAlbedo) { setUniform3fv(m_diffuseAlbedoUHandle, diffuseAlbedo.get_value()); }
    void setSpecularAlbedo(const nv::vec3f& specularAlbedo) { setUniform3fv(m_specularAlbedoUHandle, specularAlbedo.get_value()); }
    void setSpecularPower(const float& specularPower) { setUniform1f(m_specularPowerUHandle, specularPower); }

private:
    GLint m_positionAttrHandle;
    GLint m_normalAttrHandle;
    GLint m_modelMatrixUHandle;
    GLint m_viewMatrixUHandle;
    GLint m_projMatrixUHandle;
    GLint m_lightDirUHandle;
    GLint m_lightVPSBMatricesUHandle;
    GLint m_normalizedFarPlanesUHandle;
    GLint m_ambientUHandle;
    GLint m_diffuseAlbedoUHandle;
    GLint m_specularAlbedoUHandle;
    GLint m_specularPowerUHandle;
};