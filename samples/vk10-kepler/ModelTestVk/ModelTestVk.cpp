//----------------------------------------------------------------------------------
// File:        vk10-kepler\ModelTestVk/ModelTestVk.cpp
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
#include "ModelTestVk.h"
#include "NvAppBase/NvInputTransformer.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvModel/NvModelExt.h"
#include "NvModel/NvModel.h"
#include "NvVkUtil/NvModelVK.h"
#include "NvVkUtil/NvModelExtVK.h"
#include "NvVkUtil/NvQuadVK.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"

#define ARRAY_SIZE(a) ( sizeof(a) / sizeof( (a)[0] ))

using namespace Nv;

enum {
	CYCLE_MODEL = 1024
};

// Simple loader implementation for NvModelExt
class ModelTestVkLoader : public NvModelFileLoader
{
public:
	ModelTestVkLoader() {}
	virtual ~ModelTestVkLoader() {}
	virtual char* LoadDataFromFile(const char* fileName)
	{
		int32_t length;
		return NvAssetLoaderRead(fileName, length);
	}

	virtual void ReleaseData(char* pData)
	{
		NvAssetLoaderFree(pData);
	}
};


ModelTestVk::ModelTestVk()
{
	m_transformer->setTranslationVec(nv::vec3f(0.0f, 0.0f, -3.0f));
	m_transformer->setRotationVec(nv::vec3f(NV_PI*0.35f, 0.0f, 0.0f));

	// Required in all subclasses to avoid silent link issues
	forceLinkHack();
}

ModelTestVk::~ModelTestVk()
{
	LOGI("ModelTestVk: destroyed\n");
}

void ModelTestVk::initRendering(void) {
	VkResult result;
	NvAssetLoaderAddSearchPath("vk10-kepler/ModelTestVk");

	/////////////////
	// Setup textures
	NvVkTexture tex;
	
	vk().uploadTextureFromDDSFile("textures/sky_cube.dds", tex);
	
	// Create the sampler
	VkSamplerCreateInfo samplerCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.mipLodBias = 0.0;
	samplerCreateInfo.maxAnisotropy = 1;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerCreateInfo.minLod = 0.0;
	samplerCreateInfo.maxLod = 16.0;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	VkSampler sampler;
	result = vkCreateSampler(device(), &samplerCreateInfo, 0, &sampler);
	CHECK_VK_RESULT();

	//////////////
	// Load Models

	mModelCount = 3;
	mModels = new NvModelVK*[mModelCount];
	mCurrentModel = 0;

	const char* objNames[3] = {
		"models/cow.obj",
		"models/dragon.obj",
		"models/sponza.obj"
	};

	const char* nvmNames[3] = {
		"models/cow.nvm",
		"models/dragon.nvm",
		"models/sponza.nvm"
	};

	for (uint32_t i = 0; i < mModelCount; i++) {
		int32_t length;
		char *modelData = NvAssetLoaderRead(nvmNames[i], length);
		NvModelVK* model = NvModelVK::CreateFromPreprocessed(vk(), (uint8_t *)modelData);
		mModels[i] = model;
		NvAssetLoaderFree(modelData);
	}

	{
		ModelTestVkLoader loader;
		NvModelExt::SetFileLoader(&loader);
		NvModelExt* pModel = NvModelExt::CreateFromPreprocessed("models/sponza.nve");
		mModelExt = NvModelExtVK::Create(vk(), pModel);

		nv::vec3f extents = (pModel->GetMaxExt() - pModel->GetMinExt()) * 0.5f;
		float radius = std::max(extents.x, std::max(extents.y, extents.z));

		mModelExtModelTransform.set_scale(1 / radius);
		mModelExtModelTransform.set_translate((-pModel->GetCenter()) / radius);
	}

	uint32_t matCount = mModelExt->GetMaterialCount();

	mMatBlockStride = sizeof(MaterialBlock);
	mMatBlockStride = (mMatBlockStride + 255) & ~255;
	uint32_t matBlockSize = matCount * mMatBlockStride;

	result = vk().createAndFillBuffer(matBlockSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, mMatBlockBuffer);
	CHECK_VK_RESULT();

	uint8_t* matPtr;
	result = vkMapMemory(device(), mMatBlockBuffer.mem, 0, matBlockSize, 0, (void**)&matPtr);
	CHECK_VK_RESULT();

	for (uint32_t i = 0; i < matCount; i++) {
		MaterialBlock& matBlock = *((MaterialBlock*)(matPtr + i * mMatBlockStride));
		NvMaterialVK* mat = mModelExt->GetMaterial(i);

		nv::vec3f amb(mat->m_ambient);
		if (i == 0)
			matBlock.mAmbient = nv::vec4f(0.15f, 0.15f, 0.15f, 1.0f);
		else
			matBlock.mAmbient = nv::vec4f(amb.x, amb.y, amb.z, 1.0f);

		nv::vec3f diff(mat->m_diffuse);

		if (i == 0)
			matBlock.mDiffuse = nv::vec4f(0.85f, 0.85f, 0.85f, 1.0f);
		else
			matBlock.mDiffuse = nv::vec4f(diff.x, diff.y, diff.z, 1.0f);
	}
	

	// Create descriptor layout to match the shader resources
	VkDescriptorSetLayoutBinding binding[3];
	binding[0].binding = 0;
	binding[0].descriptorCount = 1;
	binding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	binding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	binding[0].pImmutableSamplers = NULL;
	binding[1].binding = 1;
	binding[1].descriptorCount = 1;
	binding[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	binding[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	binding[1].pImmutableSamplers = NULL;
	binding[2].binding = 2;
	binding[2].descriptorCount = 1;
	binding[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	binding[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	binding[2].pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo descriptorSetEntry = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptorSetEntry.bindingCount = 3;
	descriptorSetEntry.pBindings = binding;

	result = vkCreateDescriptorSetLayout(device(), &descriptorSetEntry, 0, &mDescriptorSetLayout);
	CHECK_VK_RESULT();

	// Create descriptor region and set
	VkDescriptorPoolSize descriptorPoolInfo[2];

	descriptorPoolInfo[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptorPoolInfo[0].descriptorCount = 3;
	descriptorPoolInfo[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolInfo[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo descriptorRegionInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorRegionInfo.maxSets = 4;
	descriptorRegionInfo.poolSizeCount = 2;
	descriptorRegionInfo.pPoolSizes = descriptorPoolInfo;
	VkDescriptorPool descriptorRegion;
	result = vkCreateDescriptorPool(device(), &descriptorRegionInfo, NULL, &descriptorRegion);
	CHECK_VK_RESULT();

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptorSetAllocateInfo.descriptorPool = descriptorRegion;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &mDescriptorSetLayout;
	result = vkAllocateDescriptorSets(device(), &descriptorSetAllocateInfo, &mDescriptorSet);
	CHECK_VK_RESULT();


	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &mDescriptorSetLayout;
	result = vkCreatePipelineLayout(device(), &pipelineLayoutCreateInfo, 0, &mPipelineLayout);
	CHECK_VK_RESULT();

	// Create static state info for the mPipeline.

	// set dynamically
	VkPipelineViewportStateCreateInfo vpStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	vpStateInfo.pNext = 0;
	vpStateInfo.viewportCount = 1;
	vpStateInfo.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rsStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rsStateInfo.depthClampEnable = VK_TRUE;
	rsStateInfo.rasterizerDiscardEnable = VK_FALSE;
	rsStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rsStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rsStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	VkPipelineColorBlendAttachmentState attachments[1] = {};
	attachments[0].blendEnable = VK_FALSE;
	attachments[0].colorWriteMask = ~0;

	VkPipelineColorBlendStateCreateInfo cbStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	cbStateInfo.logicOpEnable = VK_FALSE;
	cbStateInfo.attachmentCount = ARRAY_SIZE(attachments);
	cbStateInfo.pAttachments = attachments;

	VkPipelineDepthStencilStateCreateInfo dsStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	dsStateInfo.depthTestEnable = VK_TRUE;
	dsStateInfo.depthWriteEnable = VK_TRUE;
	dsStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	dsStateInfo.depthBoundsTestEnable = VK_FALSE;
	dsStateInfo.stencilTestEnable = VK_FALSE;
	dsStateInfo.minDepthBounds = 0.0f;
	dsStateInfo.maxDepthBounds = 1.0f;

	VkPipelineMultisampleStateCreateInfo msStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	msStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	msStateInfo.alphaToCoverageEnable = VK_FALSE;
	msStateInfo.sampleShadingEnable = VK_FALSE;
	msStateInfo.minSampleShading = 1.0f;
	uint32_t smplMask = 0x1;
	msStateInfo.pSampleMask = &smplMask;

	VkPipelineTessellationStateCreateInfo tessStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
	tessStateInfo.patchControlPoints = 0;

	VkPipelineDynamicStateCreateInfo dynStateInfo;
	VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	memset(&dynStateInfo, 0, sizeof(dynStateInfo));
	dynStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynStateInfo.dynamicStateCount = 2;
	dynStateInfo.pDynamicStates = dynStates;

	// Shaders

	VkPipelineShaderStageCreateInfo shaderStages[2];
	uint32_t shaderCount = 0;
#ifdef SOURCE_SHADERS
	shaderCount = vk().createShadersFromSourceFile(
		NvAssetLoadTextFile("src_shaders/base_model.glsl"), shaderStages, 2);
#else
	{
		int32_t length;
		char* data = NvAssetLoaderRead("shaders/base_model.nvs", length);
		shaderCount = vk().createShadersFromBinaryFile((uint32_t*)data,
			length, shaderStages, 2);
	}
#endif

	mModelPipelines = new VkPipeline[mModelCount];

	for (uint32_t i = 0; i <= mModelCount; i++) {
		bool ext = i == mModelCount;
		// Create mPipeline state VI-IA-VS-VP-RS-FS-CB
		VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		// Assuming that all sub-meshes in an ModelExt have the same layout...
		pipelineInfo.pVertexInputState = ext 
			? &mModelExt->GetMesh(0)->getVIInfo() : &mModels[i]->getVIInfo();
		pipelineInfo.pInputAssemblyState = ext
			? &mModelExt->GetMesh(0)->getIAInfo() : &mModels[i]->getIAInfo();
		pipelineInfo.pViewportState = &vpStateInfo;
		pipelineInfo.pRasterizationState = &rsStateInfo;
		pipelineInfo.pColorBlendState = &cbStateInfo;
		pipelineInfo.pDepthStencilState = &dsStateInfo;
		pipelineInfo.pMultisampleState = &msStateInfo;
		pipelineInfo.pTessellationState = &tessStateInfo;
		pipelineInfo.pDynamicState = &dynStateInfo;

		pipelineInfo.stageCount = shaderCount;
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.renderPass = vk().mainRenderTarget()->clearRenderPass();
		pipelineInfo.subpass = 0;

		pipelineInfo.layout = mPipelineLayout;

		result = vkCreateGraphicsPipelines(device(), VK_NULL_HANDLE, 1, &pipelineInfo, NULL, 
			ext ? &mModelExtPipeline : (mModelPipelines + i));
		CHECK_VK_RESULT();
	}
	
	mUBO.Initialize(vk());

	m_transformer->update(0.1f);

	mUBO->mModelViewMatrix = m_transformer->getModelViewMat();

	nv::perspectiveLH(mUBO->mProjectionMatrix, 45.0f * (NV_PI / 180.0f), 16.0f / 9.0f, 1.0f, 100.0f);

	mUBO->mInvProjectionMatrix = nv::inverse(mUBO->mProjectionMatrix);

	mUBO.Update();

	mQuad = NvQuadVK::Create(vk());

	// Shaders
#ifdef SOURCE_SHADERS
	shaderCount = vk().createShadersFromSourceFile(
		NvAssetLoadTextFile("src_shaders/cube_map.glsl"), shaderStages, 2);
#else
	{
		int32_t length;
		char* data = NvAssetLoaderRead("shaders/cube_map.nvs", length);
		uint32_t shaderCount = vk().createShadersFromBinaryFile((uint32_t*)data,
			length, shaderStages, 2);
	}
#endif

	// Create mPipeline state VI-IA-VS-VP-RS-FS-CB
	VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	// Assuming that all sub-meshes in an ModelExt have the same layout...
	pipelineInfo.pVertexInputState = &mQuad->getVIInfo();
	pipelineInfo.pInputAssemblyState = &mQuad->getIAInfo();
	pipelineInfo.pViewportState = &vpStateInfo;
	pipelineInfo.pRasterizationState = &rsStateInfo;
	pipelineInfo.pColorBlendState = &cbStateInfo;
	pipelineInfo.pDepthStencilState = &dsStateInfo;
	pipelineInfo.pMultisampleState = &msStateInfo;
	pipelineInfo.pTessellationState = &tessStateInfo;
	pipelineInfo.pDynamicState = &dynStateInfo;

	pipelineInfo.stageCount = shaderCount;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.renderPass = vk().mainRenderTarget()->clearRenderPass();
	pipelineInfo.subpass = 0;

	pipelineInfo.layout = mPipelineLayout;

	result = vkCreateGraphicsPipelines(device(), VK_NULL_HANDLE, 1, &pipelineInfo, NULL,
		&mQuadPipeline);
	CHECK_VK_RESULT();



	VkDescriptorBufferInfo uboDescriptorInfo[2] = {};
	mUBO.GetDesc(uboDescriptorInfo[0]);

	uboDescriptorInfo[1].buffer = mMatBlockBuffer();
	uboDescriptorInfo[1].offset = 0;
	uboDescriptorInfo[1].range = mMatBlockStride;

	VkDescriptorImageInfo texture = {};
	texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texture.imageView = tex.view;
	texture.sampler = sampler;

	VkWriteDescriptorSet writeDescriptorSets[3];
	writeDescriptorSets[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeDescriptorSets[0].dstSet = mDescriptorSet;
	writeDescriptorSets[0].dstBinding = 0;
	writeDescriptorSets[0].dstArrayElement = 0;
	writeDescriptorSets[0].descriptorCount = 1;
	writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	writeDescriptorSets[0].pBufferInfo = uboDescriptorInfo;
	writeDescriptorSets[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeDescriptorSets[1].dstSet = mDescriptorSet;
	writeDescriptorSets[1].dstBinding = 1;
	writeDescriptorSets[1].dstArrayElement = 0;
	writeDescriptorSets[1].descriptorCount = 1;
	writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	writeDescriptorSets[1].pBufferInfo = uboDescriptorInfo+1;
	writeDescriptorSets[2] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeDescriptorSets[2].dstSet = mDescriptorSet;
	writeDescriptorSets[2].dstBinding = 2;
	writeDescriptorSets[2].dstArrayElement = 0;
	writeDescriptorSets[2].descriptorCount = 1;
	writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[2].pImageInfo = &texture;
	vkUpdateDescriptorSets(device(), 3, writeDescriptorSets, 0, 0);
}

void ModelTestVk::shutdownRendering(void) {

	// destroy other resources here
	// Free changed since last released driver - comment out for now 
}

void ModelTestVk::initUI(void) {
	if (mTweakBar) {
		//        NvTweakVarBase *var;
		NvTweakVarBase *var = mTweakBar->addButton("Change Model", CYCLE_MODEL);
		addTweakButtonBind(var, NvGamepad::BUTTON_Y);

		mTweakBar->syncValues();
	}
}

void ModelTestVk::reshape(int32_t width, int32_t height)
{
}

void ModelTestVk::draw(void)
{
	bool modelExt = mCurrentModel >= mModelCount;

	if (!modelExt)
    {
        // Single model case. Model->World transform is identity, so use transformer's matrix directly
        mUBO->mModelViewMatrix = m_transformer->getModelViewMat();
    }
    else
    {
        // Sub-model case.  Model has an initial transform that must be accounted for in the Model->World->View transform
        mUBO->mModelViewMatrix = m_transformer->getModelViewMat() * mModelExtModelTransform;
    }

	mUBO->mInvModelViewMatrix = nv::inverse(mUBO->mModelViewMatrix);

	nv::vec4f cameraLight(0.57f, 0.57f, 0.57f, 0.0f);

	nv::vec4f eyeLight = normalize(cameraLight * m_transformer->getRotationMat());

	mUBO->mModelLight[0] = eyeLight.x;
	mUBO->mModelLight[1] = eyeLight.y;
	mUBO->mModelLight[2] = eyeLight.z;

	mUBO.Update();

	// Can't bake these clears into the system at init time, as the 
	// screen/targets can be resized.
	int32_t width = getAppContext()->width();
	int32_t height = getAppContext()->height();

	VkResult result = VK_ERROR_INITIALIZATION_FAILED;

	VkCommandBuffer cmd = vk().getMainCommandBuffer();

	VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };

	renderPassBeginInfo.renderPass = vk().mainRenderTarget()->clearRenderPass();
	renderPassBeginInfo.framebuffer = vk().mainRenderTarget()->frameBuffer();
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = m_width;
	renderPassBeginInfo.renderArea.extent.height = m_height;

	VkClearValue clearValues[2];
	clearValues[0].color.float32[0] = 0.33f;
	clearValues[0].color.float32[1] = 0.44f;
	clearValues[0].color.float32[2] = 0.66f;
	clearValues[0].color.float32[3] = 1.0f;
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0;


	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.clearValueCount = 2;

	vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		VkViewport vp;
		VkRect2D sc;
		vp.x = 0;
		vp.y = 0;
		vp.height = (float)(m_height);
		vp.width = (float)(m_width);
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;

		sc.offset.x = 0;
		sc.offset.y = 0;
		sc.extent.width = vp.width;
		sc.extent.height = vp.height;

		vkCmdSetViewport(cmd, 0, 1, &vp);
		vkCmdSetScissor(cmd, 0, 1, &sc);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mQuadPipeline);
		{
			uint32_t offset[3];
			offset[0] = mUBO.getDynamicOffset();
			offset[1] = 0;
			offset[2] = 0;
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &mDescriptorSet, 1, offset);

			mQuad->Draw(cmd);
		}

		// Bind the mPipeline state
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			modelExt ? mModelExtPipeline : mModelPipelines[mCurrentModel]);
		if (!modelExt) {
			uint32_t offset[2];
			offset[0] = mUBO.getDynamicOffset();
			offset[1] = 0;
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &mDescriptorSet, 1, offset);

			mModels[mCurrentModel]->Draw(cmd);
		} else {
			uint32_t count = mModelExt->GetMeshCount();

			for (uint32_t i = 0; i < count; i++) {
				NvMeshExtVK* mesh = mModelExt->GetMesh(i);
				uint32_t index = mesh->GetMaterialID();
				uint32_t offset[2];
				offset[0] = mUBO.getDynamicOffset();
				offset[1] = mMatBlockStride * index;
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &mDescriptorSet, 1, offset);

				mesh->Draw(cmd);
			}
		}
	}
	vkCmdEndRenderPass(cmd);

	vk().submitMainCommandBuffer();
}

NvUIEventResponse ModelTestVk::handleReaction(const NvUIReaction &react)
{
	switch (react.code) {
	case CYCLE_MODEL:
		mCurrentModel++;
		if (mCurrentModel > mModelCount)
			mCurrentModel = 0;

		return nvuiEventHandled;
	}
	return nvuiEventNotHandled;
}



NvAppBase* NvAppFactory() {
	return new ModelTestVk();
}
