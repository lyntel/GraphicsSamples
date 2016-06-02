//----------------------------------------------------------------------------------
// File:        vk10-kepler\ThreadedRenderingVk/NvInstancedModelExtGL.cpp
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
#include "NvInstancedModelExtGL.h"

namespace Nv
{
	bool NvInstancedModelExtGL::EnableInstancing(uint32_t stride, NvSharedVBOGL* pInstanceDataStream) 
    {
		m_pInstanceDataStream = pInstanceDataStream;
        m_stride = stride;

		return true;
	}
    
    void NvInstancedModelExtGL::AddInstanceAttrib(GLuint attribIndex,
        GLint size,
        GLenum type,
        GLboolean normalized,
        GLsizei stride,
        GLuint divisor,
        GLuint offset)
    {
        InstanceAttribDesc desc;
        desc.m_attribIndex = attribIndex;
        desc.m_size = size;
        desc.m_type = type;
        desc.m_normalized = normalized;
        desc.m_divisor = divisor;
        desc.m_offset = offset;

        m_instanceAttributes[attribIndex] = desc;
	}

    void NvInstancedModelExtGL::RemoveInstanceAttrib(GLuint attribIndex)
    {
        m_instanceAttributes.erase(attribIndex);
    }

    uint32_t NvInstancedModelExtGL::Render(GLint positionHandle, GLint normalHandle, GLint texcoordHandle, GLint tangentHandle)
    {
        if (nullptr == m_pSourceModel)
        {
            return 0;
        }

        if ((nullptr == m_pInstanceDataStream) || m_instanceAttributes.empty())
        {
            return RenderNonInstanced(positionHandle, normalHandle, texcoordHandle, tangentHandle);
        }
        else if (m_batchSize == 0)
        {
            return RenderInstanced(positionHandle, normalHandle, texcoordHandle, tangentHandle);
        }
        else
        {
            return RenderBatched(positionHandle, normalHandle, texcoordHandle, tangentHandle);
        }
    }

	NvInstancedModelExtGL::NvInstancedModelExtGL(uint32_t instanceCount,
		NvModelExtGL* pSourceModel) :
		m_instanceCount(instanceCount),
		m_pSourceModel(nullptr)
	{
		SetSourceModel(pSourceModel);
	}

    uint32_t NvInstancedModelExtGL::RenderNonInstanced(GLint positionHandle, GLint normalHandle, GLint texcoordHandle, GLint tangentHandle)
    {
        // No instance data.  Render as single mesh.
        m_pSourceModel->DrawElements(1, positionHandle, normalHandle, texcoordHandle, tangentHandle);
        return 1;
    }

    uint32_t NvInstancedModelExtGL::RenderInstanced(GLint positionHandle, GLint normalHandle, GLint texcoordHandle, GLint tangentHandle)
    {
        // Activate the instancing data by binding the instance data stream and setting
        // up all of the offsets into each of the attributes
		intptr_t pBufferStart = m_pInstanceDataStream->getDynamicOffset();
        glBindBuffer(GL_ARRAY_BUFFER, m_pInstanceDataStream->getBuffer());
        AttribSet::iterator attribIt = m_instanceAttributes.begin();
        AttribSet::const_iterator attribEnd = m_instanceAttributes.end();
        for (; attribIt != attribEnd; ++attribIt)
        {
            const InstanceAttribDesc& desc = attribIt->second;
            glEnableVertexAttribArray(desc.m_attribIndex);
            glVertexAttribPointer(desc.m_attribIndex, desc.m_size, desc.m_type, desc.m_normalized, m_stride, (GLvoid*)(pBufferStart + desc.m_offset));
            glVertexAttribDivisor(desc.m_attribIndex, desc.m_divisor);
        }

        m_pSourceModel->DrawElements(m_instanceCount, positionHandle, normalHandle, texcoordHandle, tangentHandle);

        // Clear out the vertex attribute bindings
        attribIt = m_instanceAttributes.begin();
        for (; attribIt != attribEnd; ++attribIt)
        {
            const InstanceAttribDesc& desc = attribIt->second;
            glDisableVertexAttribArray(desc.m_attribIndex);
            glVertexAttribDivisor(desc.m_attribIndex, 0);
        }
        return 1;
    }

    uint32_t NvInstancedModelExtGL::RenderBatched(GLint positionHandle, GLint normalHandle, GLint texcoordHandle, GLint tangentHandle)
    {
        // Activate the instancing data by binding the instance data stream and setting
        // up all of the offsets into each of the attributes
		intptr_t pBufferStart = m_pInstanceDataStream->getDynamicOffset();
        bool bFirstBatch = true;
        AttribSet::iterator attribIt;
        AttribSet::const_iterator attribEnd = m_instanceAttributes.end();
        uint32_t batchOffset = 0;
        uint32_t batchInstanceCount = m_batchSize;
        uint32_t numDraws = 0;
        for (uint32_t remainingInstances = m_instanceCount; remainingInstances > 0; remainingInstances -= batchInstanceCount)
        {
            if (remainingInstances < m_batchSize)
            {
                batchInstanceCount = remainingInstances;
            }

            // We need to re-bind our array buffer, as DrawElements would have bound its own array buffer to use for its normal vertex data
            glBindBuffer(GL_ARRAY_BUFFER, m_pInstanceDataStream->getBuffer());

            attribIt = m_instanceAttributes.begin();
            for (; attribIt != attribEnd; ++attribIt)
            {
                const InstanceAttribDesc& desc = attribIt->second;
                if (bFirstBatch)
                {
                    glEnableVertexAttribArray(desc.m_attribIndex);
                    glVertexAttribDivisor(desc.m_attribIndex, desc.m_divisor);
                }

                glVertexAttribPointer(desc.m_attribIndex, desc.m_size, desc.m_type, desc.m_normalized, m_stride, (GLvoid*)(pBufferStart + batchOffset + desc.m_offset));
                CHECK_GL_ERROR();
            }

            m_pSourceModel->DrawElements(batchInstanceCount, positionHandle, normalHandle, texcoordHandle, tangentHandle);
            CHECK_GL_ERROR();

            ++numDraws;
            bFirstBatch = false;
            batchOffset += m_stride * batchInstanceCount;
        }

        // Clear out the vertex attribute bindings
        attribIt = m_instanceAttributes.begin();
        for (; attribIt != attribEnd; ++attribIt)
        {
            const InstanceAttribDesc& desc = attribIt->second;
            glDisableVertexAttribArray(desc.m_attribIndex);
            glVertexAttribDivisor(desc.m_attribIndex, 0);
        }
        return numDraws;
    }


}
