//----------------------------------------------------------------------------------
// File:        es3aep-kepler\SoftShadows/RigidMesh.cpp
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
#include "SoftShadowsCommon.h"
#include "RigidMesh.h"

#include <NV/NvPlatformGL.h>

#define HALF_FLOAT_ENUM(isES2) ((isES2) ? 0x8D61 : 0x140B) // GL_HALF_FLOAT_OES : GL_HALF_FLOAT

////////////////////////////////////////////////////////////////////////////////
// RigidMesh::RigidMesh()
////////////////////////////////////////////////////////////////////////////////
RigidMesh::RigidMesh(
    const ModelVertex *vertices,
    uint32_t vertexCount,
    const uint16_t* indices,
    uint32_t indexCount,
    bool useES2)
    : m_vertices(new Vertex[vertexCount])
    , m_vertexCount(vertexCount)
    , m_indexCount(indexCount)
    , m_vertexBuffer(0)
    , m_indexBuffer(0)
    , m_center()
    , m_extents()
    , m_useES2(useES2)
{
    if (m_vertices != 0)
    {
        // Convert all the vertices and calculate the bounding box
        float mins[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
        float maxs[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
        for (uint32_t i = 0; i < vertexCount; ++i)
        {
            m_vertices[i] = vertices[i];
            for (int j = 0; j < 3; ++j)
            {
                mins[j] = std::min(vertices[i].position[j], mins[j]);
                maxs[j] = std::max(vertices[i].position[j], maxs[j]);
            }
        }

        // Calculate the center and extents from the bounding box
        m_extents = nv::vec3f(maxs[0] - mins[0], maxs[1] - mins[1], maxs[2] - mins[2]) * 0.5f;
        m_center = nv::vec3f(mins[0], mins[1], mins[2]) + m_extents;

        // This functions copies the vertex and index buffers into their respective VBO's
        glGenBuffers(1, &m_vertexBuffer);
        glGenBuffers(1, &m_indexBuffer);

        // Stick the data for the vertices into its VBO
        glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertexCount, m_vertices, GL_STATIC_DRAW);

        // Stick the data for the indices into its VBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indexCount, indices, GL_STATIC_DRAW);

        // Clear the VBO state
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}

////////////////////////////////////////////////////////////////////////////////
// RigidMesh::~RigidMesh()
////////////////////////////////////////////////////////////////////////////////
RigidMesh::~RigidMesh()
{
    if (m_vertexBuffer != 0)
    {
        glDeleteBuffers(1, &m_vertexBuffer);
    }

    if (m_indexBuffer != 0)
    {
        glDeleteBuffers(1, &m_indexBuffer);
    }

    delete [] m_vertices;
}

////////////////////////////////////////////////////////////////////////////////
// RigidMesh::render()
////////////////////////////////////////////////////////////////////////////////
//
//             Render using Vertex Buffer Object (VBO)
//
////////////////////////////////////////////////////////////////////////////////
void RigidMesh::render(IShader &shader)
{
    // Bind the VBO for the vertex data
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);

    // Set up attribute for the position (3 floats)
    GLint positionLocation = shader.getPositionAttrHandle();
    if (positionLocation >= 0)
    {
        glVertexAttribPointer(positionLocation, 3, HALF_FLOAT_ENUM(m_useES2), GL_FALSE, sizeof(Vertex), (GLvoid *)PositionOffset);
        glEnableVertexAttribArray(positionLocation);
    }

    // Set up attribute for the normal (3 floats)
    GLint normalLocation = shader.getNormalAttrHandle();
    if (normalLocation >= 0)
    {
        glVertexAttribPointer(normalLocation, 3, HALF_FLOAT_ENUM(m_useES2), GL_FALSE, sizeof(Vertex), (GLvoid *)NormalOffset);
        glEnableVertexAttribArray(normalLocation);
    }

    // Set up the indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);

    // Do the actual drawing
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_SHORT, 0);

    // Clear state
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (positionLocation >= 0)
    {
        glDisableVertexAttribArray(positionLocation);
    }
    if (normalLocation >= 0)
    {
        glDisableVertexAttribArray(normalLocation);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
