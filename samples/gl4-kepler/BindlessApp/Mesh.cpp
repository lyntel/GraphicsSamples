//----------------------------------------------------------------------------------
// File:        gl4-kepler\BindlessApp/Mesh.cpp
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
#include "Mesh.h"
#include <NvAssert.h>
#include <algorithm>

bool      Mesh::m_enableVBUM = true;
bool      Mesh::m_setVertexFormatOnEveryDrawCall = false;
bool      Mesh::m_useHeavyVertexFormat = false;
uint32_t  Mesh::m_drawCallsPerState = 1;


////////////////////////////////////////////////////////////////////////////////
//
//  Method: Mesh::update()
//
//    This method is called to update the vertex and index data for the mesh.
//    For GL_NV_vertex_buffer_unified_memory (VBUM), we ask the driver to give us
//    GPU pointers for the buffers. Later, when we render, we use these GPU pointers
//    directly. By using GPU pointers, the driver can avoid many system memory 
//    accesses which pollute the CPU caches and reduce performance.
//
////////////////////////////////////////////////////////////////////////////////
void Mesh::update(std::vector<Vertex>& vertices, std::vector<uint16_t>& indices)
{
    if(m_vertexBuffer == 0)
    {
        glGenBuffers(1, &m_vertexBuffer);
    }

    if(m_indexCount == 0)
    {
        glGenBuffers(1, &m_indexBuffer);
    }
    
    // Stick the data for the vertices and indices in their respective buffers
    glNamedBufferDataEXT(m_vertexBuffer, sizeof(vertices[0]) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
    glNamedBufferDataEXT(m_indexBuffer, sizeof(indices[0]) * indices.size(), &indices[0], GL_STATIC_DRAW);

    // *** INTERESTING ***
    // get the GPU pointer for the vertex buffer and make the vertex buffer resident on the GPU
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glGetBufferParameterui64vNV(GL_ARRAY_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &m_vertexBufferGPUPtr); 
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &m_vertexBufferSize);
    glMakeBufferResidentNV(GL_ARRAY_BUFFER, GL_READ_ONLY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // *** INTERESTING ***
    // get the GPU pointer for the index buffer and make the index buffer resident on the GPU
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glGetBufferParameterui64vNV(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &m_indexBufferGPUPtr); 
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &m_indexBufferSize);
    glMakeBufferResidentNV(GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    m_vertexCount = int32_t(vertices.size());
    m_indexCount = int32_t(indices.size());
}



////////////////////////////////////////////////////////////////////////////////
//
//  Method: Mesh::renderPrep()
//
//    Sets up the vertex format state
//
////////////////////////////////////////////////////////////////////////////////
void Mesh::renderPrep()
{
    if(m_enableVBUM)
    {
        // Specify the vertex format
        glVertexAttribFormatNV(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex));          // Position in attribute 0 that is 3 floats
        glVertexAttribFormatNV(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex));   // Color in attribute 1 that is 4 unsigned bytes

        // Enable the relevent attributes
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        // Enable a bunch of other attributes if we're using the heavy vertex format option
        if(m_useHeavyVertexFormat == true)
        {
            glVertexAttribFormatNV(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex));
            glVertexAttribFormatNV(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex));
            glVertexAttribFormatNV(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex));
            glVertexAttribFormatNV(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex));
            glVertexAttribFormatNV(7, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex));


            glEnableVertexAttribArray(3);
            glEnableVertexAttribArray(4);
            glEnableVertexAttribArray(5);
            glEnableVertexAttribArray(6);
            glEnableVertexAttribArray(7);
        }

        // Enable Vertex Buffer Unified Memory (VBUM) for the vertex attributes
        glEnableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);
        
        // Enable Vertex Buffer Unified Memory (VBUM) for the indices
        glEnableClientState(GL_ELEMENT_ARRAY_UNIFIED_NV);
    }
    else
    {
        // For Vertex Array Objects (VAO), enable the vertex attributes
        glEnableVertexArrayAttribEXT(0, 0);
        glEnableVertexArrayAttribEXT(0, 1);

        // Enable a bunch of other attributes if we're using the heavy vertex format option
        if(m_useHeavyVertexFormat == true)
        {
            glEnableVertexArrayAttribEXT(0, 3);
            glEnableVertexArrayAttribEXT(0, 4);
            glEnableVertexArrayAttribEXT(0, 5);
            glEnableVertexArrayAttribEXT(0, 6);
            glEnableVertexArrayAttribEXT(0, 7);
        }

    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  Method: Mesh::render()
//
//    Does the actual rendering of the mesh
//
////////////////////////////////////////////////////////////////////////////////
void Mesh::render(void)
{
    NV_ASSERT(m_vertexBuffer != 0);
    NV_ASSERT(m_indexBuffer != 0);
    
    if(m_enableVBUM)
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        //             Render using Vertex Buffer Unified Memory (VBUM)
        //
        ////////////////////////////////////////////////////////////////////////////////

        // *** INTERESTING ***
        // Set up the pointers in GPU memory to the vertex attributes.
        // The GPU pointer to the vertex buffer was stored in Mesh::update() after the buffer was filled
        glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 0, 
            m_vertexBufferGPUPtr + Vertex::PositionOffset, 
            m_vertexBufferSize - Vertex::PositionOffset);
        glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 1, 
            m_vertexBufferGPUPtr + Vertex::ColorOffset,
            m_vertexBufferSize - Vertex::ColorOffset);

        // Set a bunch of other attributes if we're using the heavy vertex format option
        if(m_useHeavyVertexFormat == true)
        {
            glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 3, m_vertexBufferGPUPtr + Vertex::Attrib1Offset, m_vertexBufferSize - Vertex::Attrib1Offset);
            glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 4, m_vertexBufferGPUPtr + Vertex::Attrib2Offset, m_vertexBufferSize - Vertex::Attrib2Offset);
            glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 5, m_vertexBufferGPUPtr + Vertex::Attrib3Offset, m_vertexBufferSize - Vertex::Attrib3Offset);
            glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 6, m_vertexBufferGPUPtr + Vertex::Attrib4Offset, m_vertexBufferSize - Vertex::Attrib4Offset);
            glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 7, m_vertexBufferGPUPtr + Vertex::Attrib5Offset, m_vertexBufferSize - Vertex::Attrib5Offset);
        }

        // *** INTERESTING ***
        // Set up the pointer in GPU memory to the index buffer
        glBufferAddressRangeNV(GL_ELEMENT_ARRAY_ADDRESS_NV, 0, m_indexBufferGPUPtr, m_indexBufferSize);

        // Do the actual drawing
        for(uint32_t i=0; i<m_drawCallsPerState; i++)
        {
            glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_SHORT, 0);
        }
    }
    else
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        //             Render using Vertex Array Objects (VAO)
        //
        ////////////////////////////////////////////////////////////////////////////////

        // Set up attribute 0 for the position (3 floats)
        glVertexArrayVertexAttribOffsetEXT(0, m_vertexBuffer, 0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), Vertex::PositionOffset);

        // Set up attribute 1 for the color (4 unsigned bytes)
        glVertexArrayVertexAttribOffsetEXT(0, m_vertexBuffer, 1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), Vertex::ColorOffset);

        // Set up a bunch of other attributes if we're using the heavy vertex format option
        if(m_useHeavyVertexFormat == true)
        {
            glVertexArrayVertexAttribOffsetEXT(0, m_vertexBuffer, 3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), Vertex::Attrib1Offset);
            glVertexArrayVertexAttribOffsetEXT(0, m_vertexBuffer, 4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), Vertex::Attrib2Offset);
            glVertexArrayVertexAttribOffsetEXT(0, m_vertexBuffer, 5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), Vertex::Attrib3Offset);
            glVertexArrayVertexAttribOffsetEXT(0, m_vertexBuffer, 6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), Vertex::Attrib4Offset);
            glVertexArrayVertexAttribOffsetEXT(0, m_vertexBuffer, 7, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), Vertex::Attrib5Offset);
        }

        // Set up the indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);

        // Do the actual drawing
        for(uint32_t i=0; i<m_drawCallsPerState; i++)
        {
            glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_SHORT, 0);
        }
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  Method: Mesh::render()
//
//    Resets state related to the vertex format
//
////////////////////////////////////////////////////////////////////////////////
void Mesh::renderFinish()
{
    if(m_enableVBUM)
    {
        // Reset state
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);

        // Disable a bunch of other attributes if we're using the heavy vertex format option
        if(m_useHeavyVertexFormat == true)
        {
            glDisableVertexAttribArray(3);
            glDisableVertexAttribArray(4);
            glDisableVertexAttribArray(5);
            glDisableVertexAttribArray(6);
            glDisableVertexAttribArray(7);
        }

        glDisableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);
        glDisableClientState(GL_ELEMENT_ARRAY_UNIFIED_NV);
    }
    else
    {
        // Rendering with Vertex Array Objects (VAO)

        // Reset state
        glDisableVertexArrayAttribEXT(0, 0);
        glDisableVertexArrayAttribEXT(0, 1);

        // Disable a bunch of other attributes if we're using the heavy vertex format option
        if(m_useHeavyVertexFormat == true)
        {
            glDisableVertexArrayAttribEXT(0, 3);
            glDisableVertexArrayAttribEXT(0, 4);
            glDisableVertexArrayAttribEXT(0, 5);
            glDisableVertexArrayAttribEXT(0, 6);
            glDisableVertexArrayAttribEXT(0, 7);
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  Method: Mesh::Mesh()
//
////////////////////////////////////////////////////////////////////////////////
Mesh::Mesh(void)
{
    m_vertexBuffer = 0;
    m_indexBuffer = 0;

    m_vertexCount = 0;
    m_indexCount = 0;

    m_vertexBufferSize = 0;
    m_indexBufferSize = 0;

    m_vertexBufferGPUPtr = 0;
    m_indexBufferGPUPtr = 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Method: Mesh::~Mesh()
//
////////////////////////////////////////////////////////////////////////////////
Mesh::~Mesh(void)
{
    if(m_vertexBuffer != 0)
    {
        glDeleteBuffers(1, &m_vertexBuffer);
    }
    m_vertexBuffer = 0;

    if(m_indexBuffer != 0)
    {
        glDeleteBuffers(1, &m_indexBuffer);
    }
    m_indexBuffer = 0;
}




////////////////////////////////////////////////////////////////////////////////
//
//  Method: Vertex::Vertex()
//
////////////////////////////////////////////////////////////////////////////////
Vertex::Vertex(float x, float y, float z, float r, float g, float b, float a)
{
    m_position[0] = x;
    m_position[1] = y;
    m_position[2] = z;
    m_color[0] = (uint8_t)(std::max(std::min(r, 1.0f), 0.0f) * 255.5f);
    m_color[1] = (uint8_t)(std::max(std::min(g, 1.0f), 0.0f) * 255.5f);
    m_color[2] = (uint8_t)(std::max(std::min(b, 1.0f), 0.0f) * 255.5f);
    m_color[3] = (uint8_t)(std::max(std::min(a, 1.0f), 0.0f) * 255.5f);
}


