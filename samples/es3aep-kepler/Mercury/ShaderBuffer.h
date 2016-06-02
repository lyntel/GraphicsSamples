//----------------------------------------------------------------------------------
// File:        es3aep-kepler\Mercury/ShaderBuffer.h
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
#ifndef SHADER_BUFFER_H 
#define SHADER_BUFFER_H
/*
    Simple class to represent OpenGL shader storage buffer object
*/

#include "NV/NvPlatformGL.h"
#include <iostream> 

template <class T> 
class ShaderBuffer {
public:
    ShaderBuffer(size_t size, GLenum usage = GL_STATIC_DRAW);  // size in number of elements
    ~ShaderBuffer();

    void bind();
    void unbind();

    T *map(GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    void unmap();

    GLuint getBuffer() { return m_buffer; }
    size_t getSize() const { return m_size; }

    void dump();

private:
    static const GLenum target = GL_SHADER_STORAGE_BUFFER;

    size_t m_size;

    GLuint m_buffer;
};

template <class T> 
ShaderBuffer<T>::ShaderBuffer(size_t size, GLenum usage) :
m_size(size)
{
    glGenBuffers(1, &m_buffer);

    bind();
    glBufferData(target, m_size * sizeof(T), 0, usage);
    unbind();
}

template <class T> 
ShaderBuffer<T>::~ShaderBuffer()
{
    glDeleteBuffers(1, &m_buffer);
}

template <class T>
void ShaderBuffer<T>::bind()
{
    glBindBuffer(target, m_buffer);
}

template <class T>
void ShaderBuffer<T>::unbind()
{
    glBindBuffer(target, 0);
}

template <class T>
T *ShaderBuffer<T>::map(GLbitfield access)
{
    bind();
    return (T *) glMapBufferRange(target, 0, m_size*sizeof(T), access);
}

template <class T>
void ShaderBuffer<T>::unmap()
{
    bind();
    glUnmapBuffer(target);
}

template <class T>
void ShaderBuffer<T>::dump()
{
    T *data = map(GL_MAP_READ_BIT);
    for(size_t i=0; i<m_size; i++) {
        std::cout << i << ": " << data[i] << std::endl;
    }
    unmap();
}

#endif // SHADER_BUFFER_H