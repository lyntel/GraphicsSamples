//----------------------------------------------------------------------------------
// File:        vk10-kepler\ThreadedRenderingVk/NvSharedVBOVK.cpp
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
#include "NvSharedVBOVK.h"
#include "NvVkUtil/NvVkContext.h"
#include <NV/NvLogs.h>

namespace Nv
{
	NvSharedVBOVK::NvSharedVBOVK()
		: m_vk(nullptr)
		, m_index(0)
		, m_numBuffers(0)
		, m_dataSize(0)
		, m_bufferSize(0)
		, m_vboData(nullptr)
		, m_offset(0)
	{}

	bool NvSharedVBOVK::Initialize(NvVkContext* vk, uint32_t dataSize,
		uint32_t numBuffers, VkFlags usage, VkFlags memProps)
	{
		VkResult result;
		// Initialize all of the basic bookkeeping
		m_vk = vk;
		m_numBuffers = numBuffers;
		m_index = 0;
		m_offset = 0;

		// Round the buffer size up to multiples of 4 bytes
		m_dataSize = (dataSize + 3) & 0xFFFFFFFC;

		m_bufferSize = m_dataSize * m_numBuffers;

		// Create the vertex buffer
		result = vk->createAndFillBuffer(m_bufferSize,
			usage, memProps, m_vbo);
		if (result != VK_SUCCESS)
			return false;

		result = vkMapMemory(vk->device(), m_vbo.mem, 0, m_bufferSize, 0, (void**)&m_vboData);
		if (result != VK_SUCCESS)
			return false;

		return true;
	}

	bool NvSharedVBOVK::Initialize(NvVkContext* vk, uint32_t dataSize,
		uint32_t numBuffers, NvSharedVBOVKAllocator& alloc)
	{
		// Initialize all of the basic bookkeeping
		m_vk = vk;
		m_numBuffers = numBuffers;
		m_index = 0;
		m_offset = 0;

		// Round the buffer size up to multiples of 4 bytes
		m_dataSize = (dataSize + 3) & 0xFFFFFFFC;

		m_bufferSize = m_dataSize * m_numBuffers;

		if (!alloc.Alloc(m_bufferSize, m_offset, (void**)&m_vboData)) {
			return false;
		}

		m_vbo = alloc.buffer();


		return true;
	}

	void NvSharedVBOVK::Finalize()
	{
		Finish();
	}

	void NvSharedVBOVK::Finish()
	{
	}

	bool NvSharedVBOVK::BeginUpdate()
	{
		// Next buffer in the cycle
		uint32_t  nextIndex = (m_index + 1) % m_numBuffers;

		// Wait for the copy we're about to write...
//		SyncWaitResult result = m_syncs[nextIndex].Wait(m_timeout);
//		if (result == SyncWaitResult::TIMEOUT_EXPIRED) {
#ifdef _DEBUG
//			LOGI("Timed out waiting for NvSharedVBOVK sync!");
#endif
//			return false;
//		}
//		else if (result == SyncWaitResult::FAILED) {
#ifdef _DEBUG
//			LOGI("Failed waiting for NvSharedVBOVK sync!");
#endif
//			return false;
//		}

		m_index = nextIndex;

		return true;
	}

	void NvSharedVBOVK::EndUpdate() {
		VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
		range.memory = m_vbo.mem;
		range.offset = getDynamicOffset();
		range.size = m_dataSize;
		vkFlushMappedMemoryRanges(m_vk->device(), 1, &range);
	}


	void NvSharedVBOVK::DoneRendering() {
	}

}
