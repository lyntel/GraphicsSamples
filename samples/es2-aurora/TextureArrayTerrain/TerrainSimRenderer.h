//----------------------------------------------------------------------------------
// File:        es2-aurora\TextureArrayTerrain/TerrainSimRenderer.h
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

#ifndef _TERRAIN_SIM_RENDERER_
#define _TERRAIN_SIM_RENDERER_

#include <NvSimpleTypes.h>


#include "TerrainSim.h"
#include "NV/NvPlatformGL.h"
#include "KHR/khrplatform.h"

class half;

// Own vertex buffer objects (VBOs) and index buffer (IBO) for the terrain surface.  The render method
// binds them and issues the OGL draw call.
class TerrainSimRenderer
{
public:
    TerrainSimRenderer(TerrainSim *sim, const nv::vec2f& offset);
    ~TerrainSimRenderer();

    //init the Vertex Buffers
    void initBuffers();

    //update the buffer data using round-robin VBO's
    void updateBufferData();

    //render the surface
    int32_t render(GLuint posXYHandle, GLuint normalHeightHandle);

    static void* (KHRONOS_APIENTRY *glMapBufferRangePtr) (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
    static void (KHRONOS_APIENTRY *glUnmapBufferPtr) (GLenum target);

private:
    void setIndex(uint16_t *indices, int32_t i1, int32_t i2);
    size_t nInterleavedDynamicElements() const;
    size_t nInterleavedDynamicBytes() const;
    void convertDynamicAttrsToFloat(float* pOut);

    //pointer to the simulation object
    TerrainSim *m_simulation;

    // Position in world space where the grid(surface) is rendered.
    nv::vec2f m_gridRenderPos;

    // Handles to the water's XZ VBO and Index Buffers (static and do not change).
    GLuint m_positionXZVBO, m_IBO;

    // Handles to the surface normals and the height field elements interleaved.
    GLuint m_NormalsAndHeightsVBO;

    int32_t m_vertexCount, m_indexCount;
};


#endif
