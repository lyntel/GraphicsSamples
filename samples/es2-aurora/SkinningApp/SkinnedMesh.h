//----------------------------------------------------------------------------------
// File:        es2-aurora\SkinningApp/SkinnedMesh.h
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

#ifndef SKINNED_MESH_H
#define SKINNED_MESH_H

#include <NvSimpleTypes.h>

#include "Half/half.h"

#define HALF_FLOAT_ENUM(isES2) ((isES2) ? 0x8D61 : 0x140B) // GL_HALF_FLOAT_OES : GL_HALF_FLOAT


class SkinnedVertex;

class SkinnedMesh
{
public:
    int32_t          m_vertexCount;      // Number of vertices in mesh
    int32_t          m_indexCount;       // Number of indices in mesh
    uint32_t m_vertexBuffer;     // vertex buffer object for vertices
    uint32_t m_indexBuffer;      // vertex buffer object for indices
    bool         m_initialized;      // Does the mesh have data?
    bool         m_useES2;


    SkinnedMesh(void);
    ~SkinnedMesh(void);

    void render(uint32_t iPositionLocation, uint32_t iNormalLocation, uint32_t iWeightsLocation);
    void reset(void);
    void update(const SkinnedVertex* vertexData, int32_t vertexCount, const uint16_t* indices, int32_t indexCount);
};




class SkinnedVertex
{
public:
    SkinnedVertex(void);

    void setPosition(float x, float y, float z) {m_position[0] = half(x); m_position[1] = half(y); m_position[2] = half(z);}
    void setNormal(float x, float y, float z) {m_normal[0] = half(x); m_normal[1] = half(y); m_normal[2] = half(z);}
    void setWeights(float weight0, float boneIndex0, float weight1, float boneIndex1) 
    {
        m_weights[0] = half(weight0); 
        m_weights[1] = half(boneIndex0); 
        m_weights[2] = half(weight1); 
        m_weights[3] = half(boneIndex1);
    }

    half             m_position[3];
    half             m_normal[3];
    half             m_weights[4];

    static const intptr_t PositionOffset = 0;   // [ 0, 5]
    static const intptr_t NormalOffset   = 6;   // [6, 11]
    static const intptr_t WeightsOffset  = 12;  // [12, 19]
};

#endif
