//----------------------------------------------------------------------------------
// File:        vk10-kepler\ThreadedRenderingVk/NvInstancedModelExtVK.cpp
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
#include "NvInstancedModelExtVK.h"

namespace Nv
{
	bool NvInstancedModelExtVK::EnableInstancing(uint32_t stride,
		NvSharedVBOVK* pInstanceDataStream) {
		if (nullptr != m_pSourceModel) {
			m_pInstanceDataStream = pInstanceDataStream;
			if (m_pSourceModel->InstancingEnabled())
				return false;
			m_pSourceModel->EnableInstanceData(stride);
		}

		return true;
	}

	void NvInstancedModelExtVK::AddInstanceAttrib(uint32_t attribIndex,
		VkFormat format, uint32_t offset)
	{
		if (nullptr != m_pSourceModel) {
			m_pSourceModel->AddInstanceData(attribIndex, format, offset);
		}
	}

	void NvInstancedModelExtVK::BindInstanceData(VkCommandBuffer& cmd)
	{
		if (nullptr != m_pSourceModel)
		{
			VkDeviceSize offset = m_pInstanceDataStream->getDynamicOffset();
			vkCmdBindVertexBuffers(cmd, 1, 1, 
				&m_pInstanceDataStream->getBuffer().buffer, &offset);
		}
	}

	NvInstancedModelExtVK::NvInstancedModelExtVK(NvVkContext* vk, uint32_t instanceCount,
		NvModelExtVK* pSourceModel) :
		m_vk(vk),
		m_instanceCount(instanceCount),
		m_pSourceModel(nullptr)
	{
		SetSourceModel(pSourceModel);
	}

}
