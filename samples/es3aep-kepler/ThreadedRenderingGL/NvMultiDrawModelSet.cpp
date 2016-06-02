//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ThreadedRenderingGL/NvMultiDrawModelSet.cpp
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
#include "NvMultiDrawModelSet.h"
#include "NvGLUtils/NvModelExtGL.h"
#include "NvGLUtils/NvMeshExtGL.h"
#include "NvModel/NvModelSubMesh.h"
#include "BindlessTextureHelper.h"

namespace Nv
{
NvMultiDrawModelSet::NvMultiDrawModelSet(uint32_t maxUBOSize, VertexFormatBinder* pVertexBinder)
    : m_pShader(nullptr)
    , m_bindlessTextureHandles(nullptr)
    , m_numBindlessTextures(0)
    , m_vboID(0)
    , m_iboID(0)
    , m_vertexCount(0)
    , m_indexCount(0)
    , m_vertexSize(0)
    , m_positionSize(0)
    , m_positionOffset(-1)
    , m_normalSize(0)
    , m_normalOffset(-1)
    , m_texCoordSize(0)
    , m_texCoordOffset(-1)
    , m_tangentSize(0)
    , m_tangentOffset(-1)
    , m_models(nullptr)
    , m_modelCount(0)
    , m_drawCommands(nullptr)
    , m_mappedCommands(nullptr)
    , m_drawCommandsCapacity(0)
    , m_numActiveDrawCommands(0)
    , m_pVertexBinder(pVertexBinder)
    , m_instanceDataPool(nullptr)
    , m_indirectBufferID(-1)
{
    // Determine the maximum number of SchoolData structures that will 
    // fit in a UBO of the given maximum size for the current hardware.
    m_schoolsPerUBO = maxUBOSize / sizeof(SchoolData);
}

bool NvMultiDrawModelSet::SetModels(NvModelExtGL** pModels, uint32_t count)
{
    // Our first pass through the array of models will serve two purposes:
    //    1. Verify that each model has exactly one texture and one mesh
    //    2. Count up the total number of vertices and indices for all models
    //       so that we know the sizes required by our aggregate vertex/index
    //       buffers.  That way we can allocate the buffers once.
    int32_t numVerts = 0;
    int32_t numIndices = 0;
    for (uint32_t i = 0; i < count; ++i)
    {
        // In this particular case, we know that all models have exactly one mesh
        // and one texture.  
        Nv::NvModelExtGL* pModel = pModels[i];
        if (pModel->GetTextureCount() != 1)  // All of them should have a count of 1
        {
            return false;
        }

        if (pModel->GetMeshCount() != 1) // All of them should, again, have a count of 1
        {
            return false;
        }

        // Accumulate our vertex and index counts
        const Nv::NvMeshExtGL* pMesh = pModel->GetMesh(0);
        numVerts += pMesh->GetVertexCount();
        numIndices += pMesh->GetIndexCount();
    }
    m_vertexCount = numVerts;
    m_indexCount = numIndices;

    // We also know that they all have the exact same vertex
    // format.  Thus we know that they can all be coalesced into a single set
    // of buffers for a MultiDraw call.  Allocate those buffers.
    // Use the first model's mesh to determine our common vertex size and format
    const Nv::NvMeshExtGL* pMesh = pModels[0]->GetMesh(0);
    m_vertexSize = pMesh->GetVertexSize() * sizeof(float);
    m_positionSize = pMesh->GetPositionSize();
    m_positionOffset = pMesh->GetPositionOffset();
    m_normalSize = pMesh->GetNormalSize();
    m_normalOffset = pMesh->GetNormalOffset();
    m_texCoordSize = pMesh->GetTexCoordSize();
    m_texCoordOffset = pMesh->GetTexCoordOffset();
    m_tangentSize = pMesh->GetTangentSize();
    m_tangentOffset = pMesh->GetTangentOffset();

    // Allocate the shared vertex buffer
    glGenBuffers(1, &m_vboID);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboID);
    glBufferData(GL_ARRAY_BUFFER, m_vertexSize * numVerts, nullptr, GL_STATIC_DRAW);
    uint8_t* pVertData = (uint8_t*)glMapBufferRange(GL_ARRAY_BUFFER, 0, m_vertexSize * numVerts, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    // Allocate the shared index buffer
    glGenBuffers(1, &m_iboID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iboID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * numIndices, 0, GL_STATIC_DRAW);
    uint8_t* pIndexData = (uint8_t*)glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(uint32_t) * numIndices, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    // Allocate the array to hold our Bindless Textures
    m_numBindlessTextures = count;
    m_bindlessTextureHandles = new GLuint64EXT[count];

    // Allocate the array of structures that will hold information describing
    // each model's location and range in the shared buffers
    m_models = new ModelDef[count];
    m_modelCount = count;

    // Our second pass through the array of models will serve two purposes:
    //    1. Build up our array of bindless textures for the fish models, 
    //       one per model, in the same order as the fish models themselves
    //       so that we can easily index them.
    //    2. Copy each model's vertex and index data into their subrange 
    //       of the shared buffers.
    GLuint baseIndex = 0;
    GLuint baseVertex = 0;
    for (uint32_t i = 0; i < count; ++i)
    {
        // In this particular case, we know that all models have exactly one mesh
        // and one texture.  
        Nv::NvModelExtGL* pModel = pModels[i];
        m_bindlessTextureHandles[i] = BindlessTextureHelper::getTextureHandle(pModel->GetTexture(0));
        BindlessTextureHelper::makeTextureHandleResident(m_bindlessTextureHandles[i]);

        // Setup our model definition and copy our mesh data into the shared buffers
        const Nv::NvMeshExtGL* pMesh = pModel->GetMesh(0);
        {
            NV_ASSERT(m_vertexSize == (pMesh->GetVertexSize() * sizeof(float)));
            NV_ASSERT(m_positionSize == pMesh->GetPositionSize());
            NV_ASSERT(m_positionOffset == pMesh->GetPositionOffset());
            NV_ASSERT(m_normalSize == pMesh->GetNormalSize());
            NV_ASSERT(m_normalOffset == pMesh->GetNormalOffset());
            NV_ASSERT(m_texCoordSize == pMesh->GetTexCoordSize());
            NV_ASSERT(m_texCoordOffset == pMesh->GetTexCoordOffset());
            NV_ASSERT(m_tangentSize == pMesh->GetTangentSize());
            NV_ASSERT(m_tangentOffset == pMesh->GetTangentOffset());
        }

        ModelDef& modelDef = m_models[i];
        modelDef.count = pMesh->GetIndexCount();
        modelDef.baseIndex = baseIndex;
        modelDef.baseVertex = baseVertex;

        // We need access to the raw vertex and index data
        const Nv::SubMesh* pSubMesh = pMesh->GetSubMesh();
        uint8_t* pDestVerts = pVertData + (baseVertex * m_vertexSize);
        memcpy(pDestVerts, pSubMesh->getVertices(), pMesh->GetVertexCount() * m_vertexSize);
        baseVertex += pMesh->GetVertexCount();

        uint8_t* pDestIndices = pIndexData + (baseIndex * sizeof(uint32_t));
        memcpy(pDestIndices, pSubMesh->getIndices(), modelDef.count * sizeof(uint32_t));
        baseIndex += modelDef.count;
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return true;
}

void NvMultiDrawModelSet::SetShader(NvGLSLProgram* pShader)
{
    m_pShader = pShader;
    if (nullptr == pShader)
    {
        return;
    }

    // Retrieve uniform locations for parameters that need to be set for each draw
    m_startingSchoolLocation = m_pShader->getUniformLocation("u_startingSchool");

    // The UBO for the SchoolData array is at a fixed location specified by the shader
    m_uboLocation = 5;
}

void NvMultiDrawModelSet::SetInstanceDataPool(NvSharedVBOGLPool* pInstanceData)
{
    m_instanceDataPool = pInstanceData;
}

void NvMultiDrawModelSet::SetNumSchools(uint32_t numSchools)
{
    // Do we need to grow our draw command buffer?
    if (numSchools > m_drawCommandsCapacity)
    {
        // Yes
        DrawElementsIndirectCommand* pNewDrawCommands = new DrawElementsIndirectCommand[numSchools];

        // If we have previously defined draw commands, we'll need to copy them over to the new buffer
        // before we delete the old one
        uint32_t oldCommandsSize = sizeof(DrawElementsIndirectCommand) * m_drawCommandsCapacity;
        uint32_t newCommandsSize = sizeof(DrawElementsIndirectCommand) * numSchools;
        if (nullptr != m_drawCommands)
        {
            memcpy(pNewDrawCommands, m_drawCommands, sizeof(DrawElementsIndirectCommand) * m_drawCommandsCapacity);

            // Delete the old buffer
            delete[] m_drawCommands;
        }

        // Zero out the rest of the new buffer
        memset((uint8_t*)pNewDrawCommands + oldCommandsSize, 0, newCommandsSize - oldCommandsSize);

        m_drawCommands = pNewDrawCommands;

        // If we had a previously mapped buffer, then we need to delete it and create a new one,
        // as buffers allocated with glBufferStorage are immutable and can not be resized
        if (nullptr != m_mappedCommands)
        {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBufferID);
            glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
            glDeleteBuffers(1, &m_indirectBufferID);
        }

        glGenBuffers(1, &m_indirectBufferID);
        
        // Create the actual buffer object
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBufferID);
        glBufferStorage(GL_DRAW_INDIRECT_BUFFER, newCommandsSize, m_drawCommands, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

        // Remap the buffer's memory
        m_mappedCommands = (DrawElementsIndirectCommand*)glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, newCommandsSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

        // Update our capacity to the new value
        m_drawCommandsCapacity = numSchools;
    }

    // See if we need to increase the number of UBOs
    uint32_t currentUBOCapacity = m_ubos.size() * m_schoolsPerUBO;
    uint32_t bufferSize = m_schoolsPerUBO * sizeof(SchoolData);
    while (currentUBOCapacity < numSchools)
    {
        // We need to add a new UBO buffer for more schools
        UBO ubo;
        glGenBuffers(1, &ubo.m_uboID);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo.m_uboID);

        // Allocate the new buffer
        glBufferStorage(GL_UNIFORM_BUFFER, bufferSize, (void*)0, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

        // Map the buffer so that school data can later be written into it
        ubo.m_pData = (SchoolData*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, bufferSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        memset(ubo.m_pData, 0, bufferSize);
        m_ubos.push_back(ubo);

        currentUBOCapacity += m_schoolsPerUBO;
    }

    m_numActiveDrawCommands = numSchools;
}

bool NvMultiDrawModelSet::UpdateSchoolData(School* pSchool, uint32_t baseInstance)
{
    uint32_t index = pSchool->GetIndex();
    if (index >= m_drawCommandsCapacity)
    {
        // Invalid index.  Set the number of schools first.
        return false;
    }

    // Update the information required for the school's draw command
    DrawElementsIndirectCommand& cmd = m_drawCommands[index];
    ModelDef& modelDef = m_models[pSchool->GetModelIndex()];
    cmd.count = modelDef.count;
    cmd.baseIndex = modelDef.baseIndex;
    cmd.baseVertex = modelDef.baseVertex;
    cmd.instanceCount = pSchool->GetNumFish();
    cmd.baseInstance = baseInstance;

    // Copy the backing copy's values into the mapped buffer
    m_mappedCommands[index] = cmd;

    // Update the UBO data for the school
    uint32_t bufferIndex = index / m_schoolsPerUBO; // Which UBO in the array of UBOS contains the school
    if (bufferIndex < m_ubos.size())
    {
        uint32_t subIndex = index % m_schoolsPerUBO;    // Which entry in that UBO matches the school
        SchoolData* pSchoolData = m_ubos[bufferIndex].m_pData + subIndex;
        pSchoolData->m_modelMatrix = pSchool->GetModelMatrix();
        pSchoolData->m_tailStart = pSchool->GetTailStart();
        // Index in the Texture array is the same as the model index
        uint64_t textureHandle = m_bindlessTextureHandles[pSchool->GetModelIndex()];
        pSchoolData->m_textureHandle = textureHandle;
    }
    return true;
}

uint32_t NvMultiDrawModelSet::Render(GLint positionHandle, GLint normalHandle, GLint texcoordHandle, GLint tangentHandle)
{
    uint32_t drawCount = 0;

    // Bind our vertex/index buffers
    glBindBuffer(GL_ARRAY_BUFFER, m_vboID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iboID);

    // Set up the vertex format for our main geometry vertex buffer
    glVertexAttribPointer(positionHandle, m_positionSize, GL_FLOAT, GL_FALSE, m_vertexSize, (GLvoid*)m_positionOffset);
    glEnableVertexAttribArray(positionHandle);

    if ((normalHandle >= 0) && (m_normalSize > 0))
    {
        glVertexAttribPointer(normalHandle, m_normalSize, GL_FLOAT, GL_FALSE, m_vertexSize, (GLvoid*)m_normalOffset);
        glEnableVertexAttribArray(normalHandle);
    }

    if ((texcoordHandle >= 0) && (m_texCoordSize > 0))
    {
        glVertexAttribPointer(texcoordHandle, m_texCoordSize, GL_FLOAT, GL_FALSE, m_vertexSize, (GLvoid*)m_texCoordOffset);
        glEnableVertexAttribArray(texcoordHandle);
    }

    if ((tangentHandle >= 0) && (m_tangentSize > 0))
    {
        glVertexAttribPointer(tangentHandle, m_tangentSize, GL_FLOAT, GL_FALSE, m_vertexSize, (GLvoid*)m_tangentOffset);
        glEnableVertexAttribArray(tangentHandle);
    }

    // Activate the GL Draw Indirect buffer
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBufferID);

    // Activate our instance data
    m_pVertexBinder->Activate(m_instanceDataPool);

    // Our draw offset will always begin at 0
    uint32_t baseDraw = 0;

    // We will always draw all of our currently active draw commands
    uint32_t remainingDrawCommands = m_numActiveDrawCommands;
    NV_ASSERT(remainingDrawCommands <= (m_ubos.size() * m_schoolsPerUBO));

    for (uint32_t uboIndex = 0; remainingDrawCommands > 0; ++uboIndex)
    {
        // We can execute a maximum number (m_schoolsPerUBO) of draw commands with each draw call
        uint32_t numDraws = (remainingDrawCommands > m_schoolsPerUBO) ? m_schoolsPerUBO : remainingDrawCommands;

        // For each multi draw call, activate the UBO that contains the per-school data for this batch of schools
        glBindBufferBase(GL_UNIFORM_BUFFER, m_uboLocation, m_ubos[uboIndex].m_uboID);

        // And set the starting index for the schools in this call
        glUniform1ui(m_startingSchoolLocation, baseDraw);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(baseDraw * sizeof(DrawElementsIndirectCommand)), numDraws, sizeof(DrawElementsIndirectCommand));
        baseDraw += numDraws;
        remainingDrawCommands -= numDraws;
        ++drawCount;
    }

    m_pVertexBinder->Deactivate();

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Unbind our vertex/index buffers
    glDisableVertexAttribArray(positionHandle);
    if (normalHandle >= 0)
    {
        glDisableVertexAttribArray(normalHandle);
    }
    if (texcoordHandle >= 0)
    {
        glDisableVertexAttribArray(texcoordHandle);
    }
    if (tangentHandle >= 0)
    {
        glDisableVertexAttribArray(tangentHandle);
    }

    // Unbind our vertex/index buffers
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return drawCount;
}
}