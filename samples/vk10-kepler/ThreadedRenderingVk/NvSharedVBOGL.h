//----------------------------------------------------------------------------------
// File:        vk10-kepler\ThreadedRenderingVk/NvSharedVBOGL.h
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
#ifndef NVSHAREDVBO_GL_H_
#define NVSHAREDVBO_GL_H_

#include <NvSimpleTypes.h>
#include <NV/NvPlatformGL.h>

//#include "NvAppBase/NvThread.h"

namespace Nv
{
	/// \file
	/// GL-specific wrapper for a Vertex Buffer object intended to be written
	/// each frame by code running on the CPU. Implements a ring-buffer of VBOs 
	/// that are synchronized with GPU frames so that a data pointer may be
	/// retrieved each frame and used to fill in the vertex buffer for that
	/// frame's rendering. The shared VBO may have n buffers, allowing up to n
	/// frames worth of updates before attempting to update the buffer will
	/// cause a wait until the GPU has finished consuming one of the buffers.
	class NvSharedVBOGL
	{
	public:
        NvSharedVBOGL();
        ~NvSharedVBOGL(){}

        bool Initialize(uint32_t dataSize, uint32_t numBuffers);

		/// Waits for all resources to finish being used and the clean them up
		void Finalize();

		/// Waits for the queue that the shared VBO is bound to to finish
		void Finish();

		/// Updates our data pointer and ensures that the available data
		/// pointer can be modified without affecting any in-flight rendering
		/// \return True if the update could begin
		bool BeginUpdate();

		void EndUpdate();

		void DoneRendering();

		/// Returns the pointer to the currently active buffer in the shared
		/// VBO
		/// \return Pointer to the current writeable range of the vertex buffer
		uint8_t* GetData()
		{
			if (nullptr == m_vboData)
			{
				return nullptr;
			}
#if USE_PERSISTENT_MAPPING
            // m_vboData always points to the beginning of the buffer, so we must
			// account for the offset of the currently used section
            return m_vboData + (m_index * m_dataSize);
#else
            // m_vboData always points to the beginning of the currently mapped
			// range of the buffer, so no additional offset is used
            return m_vboData;
#endif
		}

		/// Returns the size of each of the vertex buffers in the shared VBO
		/// \return Number of bytes per vertex buffer in the shared VBO
		uint32_t GetDataSize() const { return m_dataSize; }

		uint32_t getDynamicOffset() {
			return m_index * m_dataSize;
		}

        GLuint& getBuffer() { return m_vbo; }

	private:

		uint32_t    m_index;
		uint32_t    m_numBuffers;
		// Size, in bytes, of the data stored in the VBO buffers
		uint32_t    m_dataSize;
		// Size, in bytes, rounded up to a multiple of 4, of the VBO buffers
		uint32_t    m_bufferSize;
		GLuint		m_vbo;
		uint8_t*    m_vboData;
        GLsync*     m_fences;
	};
}
#endif // NVSHAREDVBO_GL_H_
