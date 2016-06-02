//----------------------------------------------------------------------------------
// File:        es3aep-kepler\SoftShadows/SoftShadowsCommon.h
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
#ifndef SOFT_SHADOWS_COMMON_H
#define SOFT_SHADOWS_COMMON_H

#include <NvSimpleTypes.h>

#include <NvAppBase/NvInputTransformer.h>
#include <NvAppBase/gl/NvSampleAppGL.h>
#include <NvGamepad/NvGamepad.h>
#include <NV/NvLogs.h>

#include <KHR/khrplatform.h>

#include <limits>

//--------------------------------------------------------------------------------------
struct ModelVertex
{
    float position[3];
    float normal[3];
    float color[4];
    float uv[3];
};

//--------------------------------------------------------------------------------------
struct SimpleVertex
{
    float position[3];
    float normal[3]; 
};

//--------------------------------------------------------------------------------------
inline bool createPlane(
    GLuint &indexBuffer,
    GLuint &vertexBuffer,
    float radius,
    float height)
{
    SimpleVertex vertices[] =
    {
        { { -radius, height,  radius }, { 0.0f, 1.0f, 0.0f } },
        { {  radius, height,  radius }, { 0.0f, 1.0f, 0.0f } },
        { {  radius, height, -radius }, { 0.0f, 1.0f, 0.0f } },
        { { -radius, height, -radius }, { 0.0f, 1.0f, 0.0f } },
    };
    
    uint16_t indices[] =
    {
        0, 1, 2,
        0, 2, 3,
    };

    // This functions copies the vertex and index buffers into their respective VBO's
    glGenBuffers(1, &vertexBuffer);
    glGenBuffers(1, &indexBuffer);
    
    // Stick the data for the vertices into its VBO
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * 4, vertices, GL_STATIC_DRAW);

    // Stick the data for the indices into its VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * 6, indices, GL_STATIC_DRAW);

    // Clear the VBO state
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return indexBuffer != 0 && vertexBuffer != 0;
}

//--------------------------------------------------------------------------------------
template<class T>
inline nv::vec3<T> transformCoord(const nv::matrix4<T> &m, const nv::vec3<T> &v)
{
    const T zero = static_cast<T>(0.0);
    const T one = static_cast<T>(1.0);
    nv::vec4<T> r = m * nv::vec4<T>(v, one);
    T oow = r.w == zero ? one : (one / r.w);
    return nv::vec3<T>(r) * oow;
}

//--------------------------------------------------------------------------------------
template<class T>
nv::matrix4<T>& perspectiveFrustum(nv::matrix4<T> &M, const T w, const T h, const T n, const T f)
{
    T ymax = h / (T)2.0;
    T ymin = -ymax;

    T aspect = w / h;
    T xmin = ymin * aspect;
    T xmax = ymax * aspect;

    return frustum(M, xmin, xmax, ymin, ymax, n, f);
}

//--------------------------------------------------------------------------------------
inline void transformBoundingBox(nv::vec3f bbox2[2], const nv::vec3f bbox1[2], const nv::matrix4f &matrix)
{
    bbox2[0][0] = bbox2[0][1] = bbox2[0][2] =  std::numeric_limits<float>::max();
    bbox2[1][0] = bbox2[1][1] = bbox2[1][2] = -std::numeric_limits<float>::max();
    // Transform the vertices of BBox1 and extend BBox2 accordingly
    for (int i = 0; i < 8; ++i)
    {
        nv::vec3f v(
            bbox1[(i & 1) ? 0 : 1][0],
            bbox1[(i & 2) ? 0 : 1][1],
            bbox1[(i & 4) ? 0 : 1][2]);

        nv::vec3f v1 = transformCoord(matrix, v);
        for (int j = 0; j < 3; ++j)
        {
            bbox2[0][j] = std::min(bbox2[0][j], v1[j]);
            bbox2[1][j] = std::max(bbox2[1][j], v1[j]);
        }
    }
}

#endif