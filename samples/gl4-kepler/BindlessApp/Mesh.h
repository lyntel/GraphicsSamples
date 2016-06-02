//----------------------------------------------------------------------------------
// File:        gl4-kepler\BindlessApp/Mesh.h
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
#ifndef MESH_H
#define MESH_H

#include <NvSimpleTypes.h>


#include <NV/NvPlatformGL.h>
#include <vector>

class Vertex
{
public:
    Vertex(float x, float y, float z, float r, float g, float b, float a);

    float                m_position[3];
    uint8_t              m_color[4];
    float                m_attrib0[4];
    float                m_attrib1[4];
    float                m_attrib2[4];
    float                m_attrib3[4];
    float                m_attrib4[4];
    float                m_attrib5[4];
    float                m_attrib6[4];

    static const int32_t PositionOffset = 0;
    static const int32_t ColorOffset    = 12;
    static const int32_t Attrib0Offset  = 16;
    static const int32_t Attrib1Offset  = 32;
    static const int32_t Attrib2Offset  = 48;
    static const int32_t Attrib3Offset  = 64;
    static const int32_t Attrib4Offset  = 72;
    static const int32_t Attrib5Offset  = 96;
    static const int32_t Attrib6Offset  = 112;
};


class Mesh
{
public:
    int32_t         m_vertexCount;            // Number of vertices in mesh
    int32_t         m_indexCount;             // Number of indices in mesh
    GLuint          m_vertexBuffer;           // vertex buffer object for vertices
    GLuint          m_indexBuffer;            // vertex buffer object for indices
    GLuint          m_paramsBuffer;           // uniform buffer object for params
    GLint           m_vertexBufferSize; 
    GLint           m_indexBufferSize;
    GLuint64EXT     m_vertexBufferGPUPtr;     // GPU pointer to m_vertexBuffer data
    GLuint64EXT     m_indexBufferGPUPtr;      // GPU pointer to m_indexBuffer data

    static bool     m_enableVBUM;
    static bool     m_setVertexFormatOnEveryDrawCall;
    static bool     m_useHeavyVertexFormat;
    static uint32_t m_drawCallsPerState;

    Mesh(void);
    ~Mesh(void);

    // TODO should NV_ASSERT in copy constructor since it can incorrectly delete the OpenGL buffer objects on copy
    
    static void renderPrep();
    static void renderFinish();

    void render(void);
    void update(std::vector<Vertex>& vertices, std::vector<uint16_t>& indices);
};

#endif
