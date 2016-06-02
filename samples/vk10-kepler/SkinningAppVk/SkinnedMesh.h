//----------------------------------------------------------------------------------
// File:        vk10-kepler\SkinningAppVk/SkinnedMesh.h
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

#ifndef SKINNED_MESH_H
#define SKINNED_MESH_H

#include <NvSimpleTypes.h>
#include "NvVkUtil/NvVkContext.h"
#include "NvVkUtil/NvSimpleUBO.h"
#include "NV/NvMath.h"
#include "Half/half.h"

class SkinnedVertex32
{
public:
	SkinnedVertex32(void);

	void setPosition(float x, float y, float z) {
		mPosition[0] = x; mPosition[1] = y; mPosition[2] = z;
	}

	void setNormal(float x, float y, float z) {
		mNormal[0] = x; mNormal[1] = y; mNormal[2] = z;
	}

	void setWeights(float weight0, float boneIndex0, float weight1, float boneIndex1) {
		mWeights[0] = weight0;
		mWeights[1] = boneIndex0;
		mWeights[2] = weight1;
		mWeights[3] = boneIndex1;
	}

	float             mPosition[3];
	float             mNormal[3];
	float             mWeights[4];

	static const int32_t msPositionOffset = 0;   // [ 0, 11]
	static const int32_t msNormalOffset = 12;   // [12, 23]
	static const int32_t msWeightsOffset = 24;  // [24, 40]
};

class SkinnedVertex
{
public:
	SkinnedVertex(void);

	void setPosition(float x, float y, float z) {
		mPosition[0] = x; mPosition[1] = y; mPosition[2] = z;
	}

	void setNormal(float x, float y, float z) {
		mNormal[0] = x; mNormal[1] = y; mNormal[2] = z;
	}

	void setWeights(float weight0, float boneIndex0, float weight1, float boneIndex1) {
		mWeights[0] = weight0;
		mWeights[1] = boneIndex0;
		mWeights[2] = weight1;
		mWeights[3] = boneIndex1;
	}

	half             mPosition[3];
	half             mNormal[3];
	half             mWeights[4];

	static const int32_t msPositionOffset = 0;   // [ 0, 5]
	static const int32_t msNormalOffset = 6;   // [6, 11]
	static const int32_t msWeightsOffset = 12;  // [12, 19]
};

class SkinnedMesh
{
public:
	struct UBOBlock {
		nv::matrix4f mMVP;
		nv::matrix4f mBones[9];
		int32_t mRenderMode[4]; // 3 used, 4th is padding
		nv::vec4f mLightDir0; // 3vec + padding.
		nv::vec4f mLightDir1;
	};

	SkinnedMesh();
    ~SkinnedMesh();

	bool initRendering(NvVkContext& vk);

	bool UpdateDescriptorSet(NvVkContext& vk);

	void draw(VkPipelineLayout& pipelineLayout, VkCommandBuffer& cmd);


	NvSimpleUBO<UBOBlock> mUBO;

	VkPipelineVertexInputStateCreateInfo mVIStateInfo;
	VkPipelineInputAssemblyStateCreateInfo mIAStateInfo;
	VkDescriptorSetLayout mDescriptorSetLayout;

protected:
	NvVkBuffer mVBO;
	NvVkBuffer mIBO;
	uint32_t mIndexCount;
	VkVertexInputAttributeDescription mAttributes[3];
	VkVertexInputBindingDescription mVertexBindings;
	VkDescriptorSet mDescriptorSet;
};




#endif
