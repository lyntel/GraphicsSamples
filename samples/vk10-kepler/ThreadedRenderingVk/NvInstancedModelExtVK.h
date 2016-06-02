//----------------------------------------------------------------------------------
// File:        vk10-kepler\ThreadedRenderingVk/NvInstancedModelExtVK.h
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
#ifndef NVINSTANCEDMODELEXT_VK_H_
#define NVINSTANCEDMODELEXT_VK_H_

#include "NvVkUtil/NvModelExtVK.h"
#include "NvSharedVBOVK.h"
#include <map>

namespace Nv
{
	/// \file
	/// VK-specific instanced set of multi-submesh geometric models; handling
	/// and rendering
	class NvInstancedModelExtVK
	{
	public:
		~NvInstancedModelExtVK()
		{
			Release();
			m_pSourceModel = nullptr;
		}

		/// Initialize instanced set of models with optional passed in ptr
		/// \param[in] alloc an allocator to use when creating the rendering
		///					 resources
		/// \param[in] pSourceModel Optional pointer to an NvModelExtVK to use
		///							for mesh data.
		///							WARNING!!! This pointer is cached in the
		///							object, and must not be freed after this
		///							call returns unless it is removed from the
		///							instanced model. WARNING!!!
		/// \return a pointer to the VK-specific object or NULL on failure
		static NvInstancedModelExtVK* Create(NvVkContext* vk, uint32_t instanceCount,
			NvModelExtVK* pSourceModel = NULL)
		{
			if (nullptr == vk)
			{
				return nullptr;
			}

			NvInstancedModelExtVK* pInstances = new NvInstancedModelExtVK(
				vk, instanceCount, pSourceModel);
			return pInstances;
		}

		/// Sets the model that is to be used as the instance template.
		/// \param[in] pSourceModel Pointer to an NvModelExtVK to use for mesh
		///							data.
		void SetSourceModel(NvModelExtVK* pSourceModel)
		{
			m_pSourceModel = pSourceModel;
		}

		/// Free the rendering resources held by this model
		void Release() {}

		/// Return the model that is being used as the instance template.
		/// \return a pointer to the #NvModelExtVK cloned by instances
		NvModelExtVK* GetModel() { return m_pSourceModel; }

		/// Retrieves the current number of instances that will be rendered
		/// when this instanced model is rendered
		/// \return Number of instances rendered by this model
		uint32_t GetInstanceCount() const { return m_instanceCount; }

		/// Sets the number of instances to render when rendering this model.
		/// \note Assumes that all instance data streams attached to this model
		/// have enough data to support the requested number of instances
		/// \param count Number of instances to render when Render() is called
		void SetInstanceCount(uint32_t count) { m_instanceCount = count; }

		bool EnableInstancing(uint32_t stride,
			NvSharedVBOVK* pInstanceDataStream);

		void AddInstanceAttrib(uint32_t attribIndex,
			VkFormat format, uint32_t offset);

		void BindInstanceData(VkCommandBuffer& cmd);

	private:
		/// \privatesection
		// NvInstancedModelExtVK may only be created through the factory
		// method
		NvInstancedModelExtVK(NvVkContext* vk,
			uint32_t instanceCount, NvModelExtVK* pSourceModel = nullptr);

		NvSharedVBOVK* m_pInstanceDataStream;

		NvVkContext* m_vk;

		// Pointer to the model to be instanced
		NvModelExtVK* m_pSourceModel;

		// Number of instances in the instance data buffer
		uint32_t m_instanceCount;
	};
}
#endif // NVINSTANCEDMODELEXT_VK_H_
