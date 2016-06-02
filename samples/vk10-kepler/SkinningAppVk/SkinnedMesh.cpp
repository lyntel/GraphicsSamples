//----------------------------------------------------------------------------------
// File:        vk10-kepler\SkinningAppVk/SkinnedMesh.cpp
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
#include "SkinnedMesh.h"
#include "CharacterModel.h"

SkinnedMesh::SkinnedMesh() {

}

SkinnedMesh::~SkinnedMesh() {
	/*
	if (mVBO() != VK_NULL_HANDLE)
		vkDestroyBuffer(device(), mVBO(), NULL);
	if (mIBO() != VK_NULL_HANDLE)
		vkDestroyBuffer(device(), mIBO(), NULL);

	if (mVBO.mem != VK_NULL_HANDLE)
		vkFreeMemory(device(), mVBO.mem, NULL);

	if (mIBO.mem != VK_NULL_HANDLE)
		vkFreeMemory(device(), mIndexBuffer.mem, NULL);
		*/
}

bool SkinnedMesh::initRendering(NvVkContext& vk) {
	VkResult result;

	// Initialize the mesh
	int32_t vertexCount = sizeof(gCharacterModelVertices) / (10 * sizeof(float)); // 3x pos, 3x norm, (2+2)x bone
	mIndexCount = sizeof(gCharacterModelIndices) / (sizeof(gCharacterModelIndices[0]));

	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(vk.physicalDevice(), 
		VK_FORMAT_R16G16B16_SFLOAT, &formatProperties);

	bool supportsRGB16 = !!(formatProperties.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT);

	if (supportsRGB16) {
		// Convert the float data in the vertex array to half data
		{
			int32_t index = 0;
			for (int32_t i = 0; i < vertexCount; i++)
			{
				SkinnedVertex& v = ((SkinnedVertex*)gCharacterModelVertices)[i];
				v.mPosition[0] = gCharacterModelVertices[index++];
				v.mPosition[1] = gCharacterModelVertices[index++];
				v.mPosition[2] = gCharacterModelVertices[index++];
				v.mNormal[0] = gCharacterModelVertices[index++];
				v.mNormal[1] = gCharacterModelVertices[index++];
				v.mNormal[2] = gCharacterModelVertices[index++];
				v.mWeights[0] = gCharacterModelVertices[index++];
				v.mWeights[1] = gCharacterModelVertices[index++];
				v.mWeights[2] = gCharacterModelVertices[index++];
				v.mWeights[3] = gCharacterModelVertices[index++];
			}
		}
	}

	// Create a vertex buffer and fill it with data
	uint32_t vboBytes = (supportsRGB16 ? sizeof(SkinnedVertex) : sizeof(SkinnedVertex32))
		* vertexCount;

	// Create the vertex buffer
	result = vk.createAndFillBuffer(vboBytes,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mVBO, gCharacterModelVertices);
	CHECK_VK_RESULT();

	// Create the index buffer
	result = vk.createAndFillBuffer(sizeof(uint16_t) * mIndexCount,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mIBO, gCharacterModelIndices);
	CHECK_VK_RESULT();

	// Create static state info for the mPipeline.
	mVertexBindings.binding = 0;
	mVertexBindings.stride = (supportsRGB16 ? sizeof(SkinnedVertex) : sizeof(SkinnedVertex32));
	mVertexBindings.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	mAttributes[0].location = 0;
	mAttributes[0].binding = 0;
	mAttributes[1].location = 1;
	mAttributes[1].binding = 0;
	mAttributes[2].location = 2;
	mAttributes[2].binding = 0;
	if (supportsRGB16) {
		mAttributes[0].format = VK_FORMAT_R16G16B16_SFLOAT;
		mAttributes[0].offset = offsetof(SkinnedVertex, mPosition);
		mAttributes[1].format = VK_FORMAT_R16G16B16_SFLOAT;
		mAttributes[1].offset = offsetof(SkinnedVertex, mNormal);
		mAttributes[2].format = VK_FORMAT_R16G16B16A16_SFLOAT;
		mAttributes[2].offset = offsetof(SkinnedVertex, mWeights);
	}
	else {
		mAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		mAttributes[0].offset = offsetof(SkinnedVertex32, mPosition);
		mAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		mAttributes[1].offset = offsetof(SkinnedVertex32, mNormal);
		mAttributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		mAttributes[2].offset = offsetof(SkinnedVertex32, mWeights);
	}

	// Create descriptor layout to match the shader resources
	VkDescriptorSetLayoutBinding binding;
	binding.binding = 0;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	binding.pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo descriptorSetEntry = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptorSetEntry.bindingCount = 1;
	descriptorSetEntry.pBindings = &binding;

	result = vkCreateDescriptorSetLayout(vk.device(), &descriptorSetEntry, 0, &mDescriptorSetLayout);
	CHECK_VK_RESULT();

	// Create descriptor region and set
	VkDescriptorPoolSize descriptorTextures;

	descriptorTextures.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptorTextures.descriptorCount = 1;

	VkDescriptorPoolCreateInfo descriptorRegionInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorRegionInfo.maxSets = 1;
	descriptorRegionInfo.poolSizeCount = 1;
	descriptorRegionInfo.pPoolSizes = &descriptorTextures;
	VkDescriptorPool descriptorRegion;
	result = vkCreateDescriptorPool(vk.device(), &descriptorRegionInfo, 0, &descriptorRegion);
	CHECK_VK_RESULT();

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptorSetAllocateInfo.descriptorPool = descriptorRegion;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &mDescriptorSetLayout;
	result = vkAllocateDescriptorSets(vk.device(), &descriptorSetAllocateInfo, &mDescriptorSet);
	CHECK_VK_RESULT();

	mVIStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	mVIStateInfo.vertexBindingDescriptionCount = 1;
	mVIStateInfo.pVertexBindingDescriptions = &mVertexBindings;
	mVIStateInfo.vertexAttributeDescriptionCount = 3;
	mVIStateInfo.pVertexAttributeDescriptions = mAttributes;

	mIAStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	mIAStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	mIAStateInfo.primitiveRestartEnable = VK_FALSE;


	mUBO.Initialize(vk);

	return true;
}

bool SkinnedMesh::UpdateDescriptorSet(NvVkContext& vk) {
	VkDescriptorBufferInfo uboDescriptorInfo = {};
	mUBO.GetDesc(uboDescriptorInfo);

	VkWriteDescriptorSet writeDescriptorSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeDescriptorSet.dstSet = mDescriptorSet;
	writeDescriptorSet.dstBinding = 0;
	writeDescriptorSet.dstArrayElement = 0;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	writeDescriptorSet.pBufferInfo = &uboDescriptorInfo;
	vkUpdateDescriptorSets(vk.device(), 1, &writeDescriptorSet, 0, 0);

	return true;
}

void SkinnedMesh::draw(VkPipelineLayout& pipelineLayout, VkCommandBuffer& cmd) {
	VkResult result = VK_SUCCESS;

	uint32_t offset = mUBO.getDynamicOffset();
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &mDescriptorSet, 1, &offset);

	// Bind the vertex and index buffers
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, &mVBO(), offsets);
	vkCmdBindIndexBuffer(cmd, mIBO(), 0, VK_INDEX_TYPE_UINT16);

	// Draw the triangle
	vkCmdDrawIndexed(cmd, mIndexCount, 1, 0, 0, 0);
}





////////////////////////////////////////////////////////////////////////////////
//
//                              Vertex methods
//
////////////////////////////////////////////////////////////////////////////////
SkinnedVertex::SkinnedVertex(void)
{
}

SkinnedVertex32::SkinnedVertex32(void)
{
}
