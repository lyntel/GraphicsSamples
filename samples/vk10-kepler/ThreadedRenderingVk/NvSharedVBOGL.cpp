//----------------------------------------------------------------------------------
// File:        vk10-kepler\ThreadedRenderingVk/NvSharedVBOGL.cpp
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
#include <NvSharedVBOGL.h>
//#include "NvVkUtil/NvVkContext.h"
#include <NV/NvLogs.h>

#define USE_PERSISTENT_MAPPING 0

namespace Nv
{
    NvSharedVBOGL::NvSharedVBOGL()
        : m_index(0)
        , m_numBuffers(0)
        , m_dataSize(0)
        , m_bufferSize(0)
        , m_vboData(nullptr)
        , m_fences(nullptr)
    {}

    bool NvSharedVBOGL::Initialize(uint32_t dataSize, uint32_t numBuffers)
    {
        // Initialize all of the basic bookkeeping
        m_numBuffers = numBuffers;
        m_index = 0;

        // Round the buffer size up to multiples of 4 bytes
        m_dataSize = (dataSize + 3) & 0xFFFFFFFC;

        m_bufferSize = m_dataSize * m_numBuffers;

        // Create the vertex buffer
        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
#if USE_PERSISTENT_MAPPING
        GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glBufferStorage(GL_ARRAY_BUFFER, m_bufferSize, 0, flags);
        m_vboData = static_cast<uint8_t*>(glMapBufferRange(GL_ARRAY_BUFFER, 0, m_bufferSize, flags));
        if (nullptr == m_vboData)
        {
            return false;
        }
#else
        glBufferData(GL_ARRAY_BUFFER, m_bufferSize, 0, GL_STREAM_DRAW);
#endif
        // Allocate space for a synchronization fence for each buffer
        m_fences = new GLsync[m_numBuffers];
        for (uint32_t i = 0; i < m_numBuffers; ++i) { m_fences[i] = nullptr; }

        return true;
    }

    void NvSharedVBOGL::Finalize()
    {
        Finish();
    }

    void NvSharedVBOGL::Finish()
    {
        // Wait on all fences and release them as they complete
        if (nullptr != m_fences)
        {
            for (uint32_t i = 0; i < m_numBuffers; ++i)
            {
                GLenum waitStatus = GL_UNSIGNALED;
                while (waitStatus != GL_ALREADY_SIGNALED && waitStatus != GL_CONDITION_SATISFIED)
                {
                    waitStatus = glClientWaitSync(m_fences[i], GL_SYNC_FLUSH_COMMANDS_BIT, 1);
                }
                glDeleteSync(m_fences[i]);
            }
            delete[] m_fences;
            m_fences = nullptr;
        }
        if (nullptr != m_vboData)
        {
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glUnmapBuffer(GL_ARRAY_BUFFER);
        }
        if (0 != m_vbo)
        {
            glDeleteBuffers(1, &m_vbo);
        }
    }

    bool NvSharedVBOGL::BeginUpdate()
    {
        // Next buffer in the cycle
        uint32_t  nextIndex = (m_index + 1) % m_numBuffers;

        // Wait for the copy we're about to write...
        if (nullptr != m_fences[nextIndex])
        {
            GLenum waitStatus = glClientWaitSync(m_fences[nextIndex], GL_SYNC_FLUSH_COMMANDS_BIT, 1);
            if (waitStatus == GL_TIMEOUT_EXPIRED)
            {
#ifdef _DEBUG
                LOGI("Timed out waiting for NvSharedVBOGL sync!");
#endif
                return false;
            }
            else if (waitStatus == GL_WAIT_FAILED) 
            {
#ifdef _DEBUG
                LOGI("Failed waiting for NvSharedVBOGL sync!");
#endif
                return false;
            }

            // Successfully waited for the fence.  Clear it and continue;
            glDeleteSync(m_fences[nextIndex]);
            m_fences[nextIndex] = nullptr;
        }

#if !USE_PERSISTENT_MAPPING
        if (0 == m_vbo)
        {
            return false;
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
        GLvoid* pBuff = glMapBufferRange(GL_ARRAY_BUFFER, m_dataSize * nextIndex, m_dataSize, flags);
        m_vboData = static_cast<uint8_t*>(pBuff);
        if (nullptr == m_vboData)
        {
            CHECK_GL_ERROR();
            return false;
        }
#endif
        m_index = nextIndex;

        return true;
    }

    void NvSharedVBOGL::EndUpdate() 
    {
#if !USE_PERSISTENT_MAPPING
        if (0 == m_vbo)
        {
            return;
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glFlushMappedBufferRange(GL_ARRAY_BUFFER, 0, m_dataSize);
        glUnmapBuffer(GL_ARRAY_BUFFER);
#endif
        m_fences[m_index] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }


    void NvSharedVBOGL::DoneRendering() 
    {
    }

}
