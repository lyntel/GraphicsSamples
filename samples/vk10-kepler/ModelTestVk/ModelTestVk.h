//----------------------------------------------------------------------------------
// File:        vk10-kepler\ModelTestVk/ModelTestVk.h
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
#include "NvVkUtil/NvSampleAppVK.h"
#include "NvVkUtil/NvSimpleUBO.h"
#include "NV/NvMath.h"

namespace Nv {
	class NvModelExtVK;
};

class NvModelVK;
class NvQuadVK;
class SimpleCommandBuffer;

class ModelTestVk : public NvSampleAppVK
{
public:
	ModelTestVk();
	~ModelTestVk();

	void initRendering(void);
	void shutdownRendering(void);
	void initUI(void);
	void draw(void);
	void reshape(int32_t width, int32_t height);

	NvUIEventResponse handleReaction(const NvUIReaction &react);

private:
	typedef struct {
		nv::matrix4f mModelViewMatrix;
		nv::matrix4f mProjectionMatrix;
		nv::matrix4f mInvModelViewMatrix;
		nv::matrix4f mInvProjectionMatrix;
		float mModelLight[3];
	} UniformBlock;

	typedef struct {
		nv::vec4f mAmbient;
		nv::vec4f mDiffuse;
	} MaterialBlock;

	NvModelVK** mModels;
	uint32_t mCurrentModel;
	uint32_t mModelCount;

	Nv::NvModelExtVK* mModelExt;
    nv::matrix4f mModelExtModelTransform;

	NvSimpleUBO<UniformBlock> mUBO;

	uint32_t mMatBlockStride;
	NvVkBuffer mMatBlockBuffer;

	NvQuadVK* mQuad;
	NvVkTexture mCubeMap;

	VkDescriptorSetLayout mDescriptorSetLayout;
	VkDescriptorSet mDescriptorSet;

	VkPipelineLayout mPipelineLayout;
	VkPipeline* mModelPipelines;
	VkPipeline mModelExtPipeline;
	VkPipeline mQuadPipeline;
};
