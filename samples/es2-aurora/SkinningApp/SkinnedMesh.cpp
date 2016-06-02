//----------------------------------------------------------------------------------
// File:        es2-aurora\SkinningApp/SkinnedMesh.cpp
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
#include "SkinnedMesh.h"
#include "NV/NvPlatformGL.h"




////////////////////////////////////////////////////////////////////////////////
//
//  Method: SkinnedMesh::render()
//
//     This does the actual skinned mesh rendering
//
////////////////////////////////////////////////////////////////////////////////
void SkinnedMesh::render(uint32_t iPositionLocation, uint32_t iNormalLocation, uint32_t iWeightsLocation)
{
    // Bind the VBO for the vertex data
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	  
    // Set up attribute for the position (3 floats)
    glVertexAttribPointer(iPositionLocation, 3, HALF_FLOAT_ENUM(m_useES2), GL_FALSE, sizeof(SkinnedVertex), (GLvoid*)SkinnedVertex::PositionOffset);
    glEnableVertexAttribArray(iPositionLocation);

    // Set up attribute for the normal (3 floats)
    glVertexAttribPointer(iNormalLocation, 3, HALF_FLOAT_ENUM(m_useES2), GL_FALSE, sizeof(SkinnedVertex), (GLvoid*)SkinnedVertex::NormalOffset);
    glEnableVertexAttribArray(iNormalLocation);

    // Set up attribute for the bone weights (4 floats)
    glVertexAttribPointer(iWeightsLocation, 4, HALF_FLOAT_ENUM(m_useES2), GL_FALSE, sizeof(SkinnedVertex), (GLvoid*)SkinnedVertex::WeightsOffset);
    glEnableVertexAttribArray(iWeightsLocation);

    // Set up the indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);

    // Do the actual drawing
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_SHORT, 0);

    // Clear state
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(iPositionLocation);
    glDisableVertexAttribArray(iNormalLocation);
    glDisableVertexAttribArray(iWeightsLocation);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}





////////////////////////////////////////////////////////////////////////////////
//
//  Method: SkinnedMesh::update()
//
//       This method copies the vertex and index buffers into their respective VBO's
//
////////////////////////////////////////////////////////////////////////////////
void SkinnedMesh::update(const SkinnedVertex* vertices, int32_t vertexCount, const uint16_t* indices, int32_t indexCount)
{
    // Check to see if we have a buffer for the vertices, if not create one
    if(m_vertexBuffer == 0)
    {
        glGenBuffers(1, &m_vertexBuffer);
    }

    // Check to see if we have a buffer for the indices, if not create one
    if(m_indexCount == 0)
    {
        glGenBuffers(1, &m_indexBuffer);
    }
    
    // Stick the data for the vertices into its VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertexCount, vertices, GL_STATIC_DRAW);

    // Stick the data for the indices into its VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indexCount, indices, GL_STATIC_DRAW);

    // Clear the VBO state
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Record mesh information
    m_vertexCount = vertexCount;
    m_indexCount = indexCount;
    m_initialized = true;
}




////////////////////////////////////////////////////////////////////////////////
//
//  Method: SkinnedMesh::reset()
//
//    Reset the state of mesh. Delete any buffers being used
//
////////////////////////////////////////////////////////////////////////////////
void SkinnedMesh::reset(void)
{
    if(m_vertexBuffer != 0)
    {
        glDeleteBuffers(1, &m_vertexBuffer);
    }
    m_vertexBuffer = 0;
    m_vertexCount = 0;

    if(m_indexBuffer != 0)
    {
        glDeleteBuffers(1, &m_indexBuffer);
    }
    m_indexBuffer = 0;    
    m_indexCount = 0;

    m_initialized = false;
}




////////////////////////////////////////////////////////////////////////////////
//
//  Method: SkinnedMesh::SkinnedMesh
//
////////////////////////////////////////////////////////////////////////////////
SkinnedMesh::SkinnedMesh(void)
{
    m_vertexBuffer = 0;
    m_indexBuffer = 0;

    m_vertexCount = 0;
    m_indexCount = 0;

    m_initialized = false;
    m_useES2 = false;
}




////////////////////////////////////////////////////////////////////////////////
//
//  Method: SkinnedMesh::~SkinnedMesh
//
////////////////////////////////////////////////////////////////////////////////
SkinnedMesh::~SkinnedMesh(void)
{
    reset();
}





////////////////////////////////////////////////////////////////////////////////
//
//                              Vertex methods
//
////////////////////////////////////////////////////////////////////////////////
SkinnedVertex::SkinnedVertex(void)
{
    m_position[0] = 0.0f;
    m_position[1] = 0.0f;
    m_position[2] = 0.0f;
    m_normal[0] = 0.0f;
    m_normal[1] = 0.0f;
    m_normal[2] = 0.0f;
    m_weights[0] = 0.0f;
    m_weights[1] = 0.0f;
    m_weights[2] = 0.0f;
    m_weights[3] = 0.0f;
}
