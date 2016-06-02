//----------------------------------------------------------------------------------
// File:        es3aep-kepler\SoftShadows/Shader.h
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
#ifndef SHADER_H
#define SHADER_H

#include <NvGLUtils/NvGLSLProgram.h>

class Shader
    : public NvGLSLProgram
{
    typedef NvGLSLProgram SuperType;

public:

    Shader(const char *vertFilename, const char* fragFilename)
        : SuperType()
    {
        setSourceFromFiles(vertFilename, fragFilename);
    }
    
protected:

    void setUniformMatrix(GLint handle, const nv::matrix4f &m)
    {
        NV_ASSERT(handle >= 0);
        glUniformMatrix4fv(handle, 1, GL_FALSE, m._array);
        CHECK_GL_ERROR();
    }

    void setUniformTexture(GLint handle, GLint unit)
    {
        NV_ASSERT(handle >= 0);
        glUniform1i(handle, unit);
        CHECK_GL_ERROR();
    }

    void setUniformBool(GLint handle, bool b)
    {
        NV_ASSERT(handle >= 0);
        glUniform1i(handle, b ? 1 : 0);
        CHECK_GL_ERROR();
    }

    void setUniformInt(GLint handle, GLint i)
    {
        NV_ASSERT(handle >= 0);
        glUniform1i(handle, i);
        CHECK_GL_ERROR();
    }

    void setUniformScalar(GLint handle, GLfloat s)
    {
        NV_ASSERT(handle >= 0);
        glUniform1f(handle, s);
        CHECK_GL_ERROR();
    }

    void setUniformVector(GLint handle, const nv::vec2f &v)
    {
        NV_ASSERT(handle >= 0);
        glUniform2fv(handle, 1, v._array);
        CHECK_GL_ERROR();
    }

    void setUniformVector(GLint handle, const nv::vec3f &v)
    {
        NV_ASSERT(handle >= 0);
        glUniform3fv(handle, 1, v._array);
        CHECK_GL_ERROR();
    }

    void setUniformVector(GLint handle, const nv::vec4f &v)
    {
        NV_ASSERT(handle >= 0);
        glUniform4fv(handle, 1, v._array);
        CHECK_GL_ERROR();
    }
};

#endif