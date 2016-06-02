//----------------------------------------------------------------------------------
// File:        es3aep-kepler\SoftShadows/RigidMesh.h
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

#ifndef RIGID_MESH_H
#define RIGID_MESH_H

#include <Half/half.h>

class RigidMesh
{
public:

    class Vertex
    {
    public:

        Vertex() {}

        Vertex &operator =(const ModelVertex &v)
        {
            m_position[0] = half(v.position[0]);
            m_position[1] = half(v.position[1]);
            m_position[2] = half(v.position[2]);

            m_normal[0] = half(v.normal[0]);
            m_normal[1] = half(v.normal[1]);
            m_normal[2] = half(v.normal[2]);

            return *this;
        }

    private:

        // Not copyable
        Vertex(const Vertex &);
        Vertex &operator =(const Vertex &);

        half m_position[3];
        half m_normal[3];
    };

    RigidMesh(
        const ModelVertex *vertices,
        uint32_t vertexCount,
        const uint16_t* indices,
        uint32_t indexCount,
        bool useES2);

    ~RigidMesh();

    uint32_t getVertexCount() const { return m_vertexCount; }
    uint32_t getIndexCount() const { return m_indexCount; }

    nv::vec3f getCenter() const { return m_center; }
    nv::vec3f getExtents() const { return m_extents; }

    struct IShader
    {
        virtual GLint getPositionAttrHandle() const = 0;
        virtual GLint getNormalAttrHandle() const = 0;
    };

    void render(IShader &shader);

    bool isValid() const { return m_vertexBuffer != 0 && m_indexBuffer != 0; }

private:

    // Not copyable
    RigidMesh(const RigidMesh &);
    RigidMesh &operator =(const RigidMesh &);

    Vertex    *m_vertices;
    uint32_t   m_vertexCount;      // Number of vertices in mesh
    uint32_t   m_indexCount;       // Number of indices in mesh
    GLuint     m_vertexBuffer;     // vertex buffer object for vertices
    GLuint     m_indexBuffer;      // vertex buffer object for indices
    nv::vec3f  m_center;
    nv::vec3f  m_extents;
    bool       m_useES2;

    static const intptr_t PositionOffset = 0;   // [0,  5]
    static const intptr_t NormalOffset   = 6;   // [6, 11]
};

#endif
