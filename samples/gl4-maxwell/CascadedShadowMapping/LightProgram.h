//----------------------------------------------------------------------------------
// File:        gl4-maxwell\CascadedShadowMapping/LightProgram.h
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

class LightProgram : public NvGLSLProgram
{
public:

    LightProgram()
        : NvGLSLProgram()
        , m_positionAttrHandle(-1)
        , m_mvpMatrixUHandle(-1)
    {
    }

    virtual bool init(const char* vertFilename,
        const char* geomFilename,
        const char* positionAttrName = "position",
        const char* mvpMatrixName = "mvpMatrix") {
            int32_t len;
            char* vertSrc = NvAssetLoaderRead(vertFilename, len);
            char* geomSrc = NvAssetLoaderRead(geomFilename, len);
            if (!vertSrc || !geomSrc) {
                NvAssetLoaderFree(vertSrc);
                NvAssetLoaderFree(geomSrc);
                return false;
            }

            ShaderSourceItem items[2];
            items[0].src = vertSrc;
            items[0].type = GL_VERTEX_SHADER;
            items[1].src = geomSrc;
            items[1].type = GL_GEOMETRY_SHADER;

            setSourceFromStrings(items, 2);

            NvAssetLoaderFree(vertSrc);
            NvAssetLoaderFree(geomSrc);

            // Vertex attributes
            m_positionAttrHandle = getAttribLocation(positionAttrName);

            // Uniforms
            m_mvpMatrixUHandle = glGetUniformLocation(NvGLSLProgram::m_program, mvpMatrixName);

            return true;
    }

    GLint getPositionAttrHandle() const { return m_positionAttrHandle; }

    void setMvpMatrix(const nv::matrix4f& mvpMatrix) { setUniformMatrix4fv(m_mvpMatrixUHandle, const_cast<float*>(mvpMatrix.get_value())); }

protected:
    GLint m_positionAttrHandle;
    GLint m_mvpMatrixUHandle;
};
