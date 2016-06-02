//----------------------------------------------------------------------------------
// File:        vk10-kepler\ThreadedRenderingVk/NvSharedVBOVK.h
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
#ifndef NVSHAREDVBO_VK_H_
#define NVSHAREDVBO_VK_H_

#include <NvSimpleTypes.h>
#include <NvVkUtil/NvVkContext.h>
//#include "NvAppBase/NvThread.h"

namespace Nv
{
	class NvSharedVBOVKAllocator {
	public:
		bool Initialize(NvVkContext& vk, uint32_t size) {
			m_size = size;
			m_offset = 0;

			VkResult result = vk.createAndFillBuffer(size,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_buffer);
			if (result != VK_SUCCESS)
				return false;

			result = vkMapMemory(vk.device(), m_buffer.mem, 0, size, 0, (void**)&m_mapped);
			if (result != VK_SUCCESS)
				return false;

			return true;
		}

		NvVkBuffer& buffer() { return m_buffer; }

		bool Alloc(uint32_t size, uint32_t& offset, void** mapped = NULL) {
			if (size + m_offset > m_size)
				return false;

			offset = m_offset;
			m_offset += size;

			if (mapped) {
				*mapped = m_mapped;
			}

			m_mapped = (void*)(((unsigned char*)m_mapped) + size);

			return true;
		}

	protected:

		NvVkBuffer m_buffer;
		uint32_t m_size;
		uint32_t m_offset;
		void* m_mapped;
	};

	/// \file
	/// VK-specific wrapper for a Vertex Buffer object intended to be written
	/// each frame by code running on the CPU. Implements a ring-buffer of VBOs 
	/// that are synchronized with GPU frames so that a data pointer may be
	/// retrieved each frame and used to fill in the vertex buffer for that
	/// frame's rendering. The shared VBO may have n buffers, allowing up to n
	/// frames worth of updates before attempting to update the buffer will
	/// cause a wait until the GPU has finished consuming one of the buffers.
	class NvSharedVBOVK
	{
	public:
		NvSharedVBOVK();
		~NvSharedVBOVK(){}

		bool Initialize(NvVkContext* vk, uint32_t dataSize,
			uint32_t numBuffers,
			VkFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VkFlags memProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

		bool Initialize(NvVkContext* vk, uint32_t dataSize,
			uint32_t numBuffers,
			NvSharedVBOVKAllocator& alloc);

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

		/// Retrieve the queue object that this VBO is bound to
		NvVkContext* vk() { return m_vk; }

		/// Returns the pointer to the currently active buffer in the shared
		/// VBO
		/// \return Pointer to the current writeable range of the vertex buffer
		uint8_t* GetData()
		{
			if (nullptr == m_vboData)
			{
				return nullptr;
			}
			return m_vboData + (m_index * m_dataSize);
		}

		/// Returns the size of each of the vertex buffers in the shared VBO
		/// \return Number of bytes per vertex buffer in the shared VBO
		uint32_t GetDataSize() const { return m_dataSize; }

		uint32_t getDynamicOffset() {
			return m_offset + m_index * m_dataSize;
		}

		NvVkBuffer& getBuffer() { return m_vbo; }

	private:

		NvVkContext* m_vk;
		uint32_t	        m_index;
		uint32_t            m_numBuffers;
		// Size, in bytes, of the data stored in the VBO buffers
		uint32_t            m_dataSize;
		// Size, in bytes, rounded up to a multiple of 4, of the VBO buffers
		uint32_t            m_bufferSize;
		NvVkBuffer			m_vbo;
		uint8_t*            m_vboData;
		uint32_t			m_offset;
	};
}
#endif // NVSHAREDVBO_VK_H_
