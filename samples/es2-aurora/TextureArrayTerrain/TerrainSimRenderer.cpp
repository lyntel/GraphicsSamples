//----------------------------------------------------------------------------------
// File:        es2-aurora\TextureArrayTerrain/TerrainSimRenderer.cpp
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

#include "TerrainSimRenderer.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NV/NvLogs.h"
#include "Half/half.h"

//#undef NDEBUG

#include "IBOBuild.h"

void* (KHRONOS_APIENTRY *TerrainSimRenderer::glMapBufferRangePtr) (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
void (KHRONOS_APIENTRY *TerrainSimRenderer::glUnmapBufferPtr) (GLenum target);

#ifndef GL_MAP_WRITE_BIT
#define GL_MAP_WRITE_BIT                2
#endif

static void checkGlError(const char* op, const char* loc) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        LOGI("after %s() glError (0x%x) @%s\n", op, error, loc);
    }
}

TerrainSimRenderer::TerrainSimRenderer(TerrainSim *sim, const nv::vec2f& offset):
    m_simulation(sim),
    m_gridRenderPos(offset),
    m_indexCount(0),
    m_vertexCount(0)
{
}

TerrainSimRenderer::~TerrainSimRenderer()
{
}

void TerrainSimRenderer::setIndex(uint16_t *indices, int32_t i1, int32_t i2)
{
    NV_ASSERT(i1 < m_indexCount);
    NV_ASSERT(i2 < m_vertexCount);
    indices[i1] = (uint16_t) i2;
}

size_t TerrainSimRenderer::nInterleavedDynamicElements() const
{
    NV_ASSERT(3 * m_simulation->totalHeightFieldElements() == m_simulation->totalNormalElements());
    return m_simulation->totalHeightFieldElements() + m_simulation->totalNormalElements();
}

size_t TerrainSimRenderer::nInterleavedDynamicBytes() const
{
    return sizeof(float) * nInterleavedDynamicElements();
}

void TerrainSimRenderer::initBuffers()
{
    const int32_t w = m_simulation->getWidth();
    const int32_t h = m_simulation->getHeight();

    // We normalize any tile to be 127/2 wide, regardless of the number of vertices.  This size
    // is purely arbitrary; 1.0 might have been a better choice.
    const float wScale = 127.0f * 0.5f / (float)(w-1);
    const float hScale = 127.0f * 0.5f / (float)(h-1);

    // This is essentially a world transform, baked into the vertex positions.
    const float xTrans = -m_gridRenderPos.x * (w-1);
    const float yTrans = -m_gridRenderPos.y * (w-1);

    m_vertexCount = w*h;
    float *surfacexz = new float[2 * m_vertexCount];
    for(int32_t i=0; i<h; i++)
    {
        for(int32_t j=0; j<w; j++)
        {
            surfacexz[i*w*2 + j*2 + 0] = hScale * ((float)i + yTrans);
            surfacexz[i*w*2 + j*2 + 1] = wScale * ((float)j + xTrans);
        }
    }

    glGenBuffers(1, &m_positionXZVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_positionXZVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * m_vertexCount, surfacexz, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkGlError("glGenBuffers m_positionXZVBO", "TerrainSimRenderer::initBuffers()");
    delete [] surfacexz;

    //no of quads for a grid of wxh = (w-1)*(h-1);
    //no of triangles = no of quads * 2;
    //no of vertices = no of triangles * 3;

    // This fn theoretically creates an optimal IB for a regular grid of quads.  I don't see any improvement
    // in perf over the previous, naive implementation.  Either I'm invoking it with incorrect params and/or
    // the bottleneck is elsewhere.
    m_indexCount = 0;
    const int32_t blockWidth = 32;
    const bool doWarm = false;
    uint16_t *indices = NULL;
    gen_planar_quad_optimised_indices(w,h, &m_indexCount, &indices, blockWidth, doWarm);
    NV_ASSERT(indices != NULL);
    NV_ASSERT(m_indexCount > 0);
    
    glGenBuffers(1, &m_IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t)*m_indexCount, indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    checkGlError("glGenBuffers m_terrainIBO", "TerrainSimRenderer::initBuffers()");
    delete [] indices;
    
    float* pFloats = new float[nInterleavedDynamicElements()];
    convertDynamicAttrsToFloat(pFloats);
    glGenBuffers(1, &m_NormalsAndHeightsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_NormalsAndHeightsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*nInterleavedDynamicElements(), pFloats, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkGlError("glGenBuffers m_gradientsVBO", "TerrainSimRenderer::initBuffers()");
    delete [] pFloats;

    LOGI("TerrainSimRenderer::initBuffers() for %d vertices, %d indices", m_vertexCount, m_indexCount);
}

void TerrainSimRenderer::updateBufferData()
{
    // If this were a real application of dynamic data - such as a terrain engine with deformation - we ought to double-buffer
    // the data to improve CPU-GPU concurrency.  Here, we only alter the data in response to user tweaks of the sliders
    // and so it only needs to be fast enough to be interactive.  We omit double-buffering for the sake of simplicity.
    glBindBuffer(GL_ARRAY_BUFFER, m_NormalsAndHeightsVBO);
    void *ptr2 = glMapBufferRangePtr(GL_ARRAY_BUFFER, 0, nInterleavedDynamicBytes(), GL_MAP_WRITE_BIT);
    int32_t error = glGetError();
    if (error || !ptr2)
        return;

    convertDynamicAttrsToFloat((float*)ptr2);
    
    glUnmapBufferPtr(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkGlError("end", "TerrainSimRenderer::updateBufferData()");

    // Too much spew:
    //LOGI("TerrainSimRenderer::updateBufferData() for %d vertices, m_current=%d", m_vertCount, m_current);
}

void TerrainSimRenderer::convertDynamicAttrsToFloat(float* pOut)
{
    const float* pHeights = m_simulation->getHeightField();
    const float* pNormals = m_simulation->getNormals();
    const float* const pEndNormals = pNormals + m_simulation->totalNormalElements();
    (void)pEndNormals; // This is only used in an NV_ASSERT, so we avoid the warning
    const float* const pEndHeights = pHeights + m_simulation->totalHeightFieldElements();

    while (pHeights < pEndHeights)
    {
        // Interleaved normal and height is more efficient than separate VBOs: half4(x,y,z, height).
        *pOut++ = *pNormals++;
        *pOut++ = *pNormals++;
        *pOut++ = *pNormals++;
        NV_ASSERT(pNormals <= pEndNormals);

        *pOut++ = *pHeights++;
    }
}

int32_t TerrainSimRenderer::render(GLuint posXYHandle, GLuint normalHeightHandle)
{
    glBindBuffer(GL_ARRAY_BUFFER, m_positionXZVBO);
    glVertexAttribPointer(posXYHandle, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, m_NormalsAndHeightsVBO);
    glVertexAttribPointer(normalHeightHandle, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkGlError("glBindBuffer vertices", "TerrainSimRenderer::render()");

    glEnableVertexAttribArray(posXYHandle);
    glEnableVertexAttribArray(normalHeightHandle);
    checkGlError("attrs bound", "TerrainSimRenderer::render()");

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
    checkGlError("glBindBuffer IBO", "TerrainSimRenderer::render()");

    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_SHORT, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDisableVertexAttribArray(posXYHandle);
    glDisableVertexAttribArray(normalHeightHandle);
    
    return m_indexCount;
}
